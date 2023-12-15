import argparse
import logging
import os
import pathlib
import platform
import shutil
import subprocess
import sys
import typing
import uuid
from concurrent.futures import ProcessPoolExecutor

import psutil
import yaml
from pydantic import BaseModel, validator

# Setup logging
# Create logger
log = logging.getLogger('build-clp-package')
log.setLevel(logging.INFO)
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter('%(asctime)s [%(levelname)s] [%(name)s] %(message)s')
logging_console_handler.setFormatter(logging_formatter)
log.addHandler(logging_console_handler)


class ClpComponent(BaseModel):
    name: str
    type: str
    url: str = None
    branch: str = None
    commit: str = None

    @validator('name', always=True)
    def component_name_validation(cls, v):
        currently_supported_component_names = [
            'package-template',
            'compression-job-handler',
            'job-orchestration',
            'clp-py-utils',
            'clp-package-utils',
            'core',
        ]
        if v not in currently_supported_component_names:
            raise ValueError(f'The specified clp component name "{v}" not supported')
        return v

    @validator('type', always=True)
    def component_type_validation(cls, v, values, **kwargs):
        if 'git' == v:
            if not values['url']:
                raise ValueError('git url must be specified')
            parameter_count = int(values['branch']) + int(values['commit'])
            if 0 == parameter_count:
                raise ValueError('git branch or commit must be specified')
            elif 2 == parameter_count:
                raise ValueError('can only specify either git branch or commit')
        elif 'local' == v:
            pass  # Nothing needs to be validated
        else:
            raise ValueError(f'The specified clp component type "{v}" not supported')
        return v


class PackagingConfig(BaseModel):
    working_dir: str
    version: str
    arch: str = platform.machine()
    artifact_name: str
    build_parallelism: int
    builder_image: str
    components: typing.List[ClpComponent]


def check_dependencies():
    try:
        subprocess.run('command -v git', shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=True)
    except subprocess.CalledProcessError:
        log.error('git is not installed on the path.')
        raise EnvironmentError

    try:
        subprocess.run('command -v docker', shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=True)
        subprocess.run(['docker', 'ps'], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=True)
    except subprocess.CalledProcessError:
        log.error('docker is not installed on the path or cannot run without superuser privileges (sudo).')
        raise EnvironmentError


def replace_clp_core_version(project_dir: pathlib.Path, version: str):
    target_replacement_line = 'constexpr char cVersion[] = '
    target_replacement_file_path = project_dir / 'src' / 'version.hpp'
    log.info(f'Updating clp core\'s version to {version} in {target_replacement_file_path}')
    with open(target_replacement_file_path, 'r') as version_file:
        version_file_lines = version_file.readlines()
    for idx, line in enumerate(version_file_lines):
        if line.startswith(target_replacement_line):
            version_file_lines[idx] = f'{target_replacement_line}"{version}";'
            break
    with open(target_replacement_file_path, 'w') as version_file:
        version_file.write('\n'.join(version_file_lines))


def clone_and_checkout(component: ClpComponent, working_dir: pathlib.Path):
    if component.branch:
        subprocess.run(['git', 'clone', '-b', component.branch, '--depth', '1', component.url, component.name],
                       cwd=working_dir, check=True)
    elif component.commit:
        subprocess.run(['git', 'clone', component.url, component.name], cwd=working_dir, check=True)
        subprocess.run(['git', 'checkout', component.commit], cwd=working_dir / component.name, check=True)


def clone_and_checkout_clp_core(component: ClpComponent, working_dir: pathlib.Path, version: str):
    clone_and_checkout(component, working_dir)

    log.info('Downloading clp core\'s submodules...')
    subprocess.run(['./download-all.sh'], cwd=working_dir / 'core' / 'tools' / 'scripts' / 'deps-download')

    replace_clp_core_version(working_dir / 'core', version)


def main(argv):
    args_parser = argparse.ArgumentParser(description='CLP package builder')
    args_parser.add_argument('--config', '-c', required=True, help='Build configuration file.')
    parsed_args = args_parser.parse_args(argv[1:])

    try:
        check_dependencies()
    except EnvironmentError:
        log.error('Unmet dependency')
        return -1

    # Parse config file
    with open(parsed_args.config, 'r') as config_file:
        try:
            packaging_config = PackagingConfig.parse_obj(yaml.safe_load(config_file))
        except:
            log.exception('Failed to parse config file.')
            return -1

    # Limit maximum build parallelization degree to minimize chance of running out of RAM
    # Minimum 2GB per core to ensure successful compilation
    if packaging_config.build_parallelism == 0:
        build_parallelization = min(int(psutil.virtual_memory().total / (2 * 1024 * 1024 * 1024)), psutil.cpu_count())
    elif packaging_config.build_parallelism > 0:
        build_parallelization = int(packaging_config.build_parallelism)
    else:
        log.error(f'Unsupported build_parallelism: {packaging_config.build_parallelism}')
        return -1

    # Infer install scripts directory
    script_dir = pathlib.Path(__file__).parent.resolve()
    host_install_scripts_dir = script_dir / 'install-scripts'
    container_install_scripts_dir = pathlib.PurePath('/tmp/install-scripts')

    # Remove existing out directory to ensure clean state prior to cloning directories
    host_working_dir = pathlib.Path(packaging_config.working_dir).resolve()
    try:
        shutil.rmtree(host_working_dir)
    except FileNotFoundError:
        pass
    except:
        log.exception(f'Failed to clean up working directory: {host_working_dir}')
        return -1

    host_working_dir.mkdir(parents=True, exist_ok=True)
    pip_cache_directory = pathlib.PurePath('/tmp')
    container_working_directory = pathlib.PurePath('/tmp/out')
    versioned_artifact_name = f'{packaging_config.artifact_name}-{packaging_config.arch}-v{packaging_config.version}'
    artifact_dir = (host_working_dir / versioned_artifact_name).resolve()

    # Download or copy source code to build working directory
    project_root = script_dir.parent.parent
    with ProcessPoolExecutor() as executor:
        for component in packaging_config.components:
            if 'git' == component.type:
                # For "git" type components, clone and checkout
                if 'core' == component.name:
                    executor.submit(clone_and_checkout_clp_core, component, host_working_dir, packaging_config.version)
                else:
                    executor.submit(clone_and_checkout, component, host_working_dir)
            elif 'local' == component.type:
                if 'core' == component.name:
                    log.info('Downloading clp core\'s submodules...')
                    cwd = project_root / 'components' / 'core' / 'tools' / 'scripts' / 'deps-download'
                    subprocess.run(['./download-all.sh'], cwd=cwd)

                # For "local" type components, copy
                shutil.copytree(project_root / 'components' / component.name, host_working_dir / component.name)

                if 'core' == component.name:
                    replace_clp_core_version(host_working_dir / component.name, packaging_config.version)

    # Make a copy of package-template/src directory and name it as the {artifact_name}-{version}
    shutil.copytree(host_working_dir / 'package-template' / 'src', artifact_dir)

    # Start build environment container
    build_environment_container_name = f'clp-builder-{uuid.uuid4()}'
    log.info(f'Starting build environment container {build_environment_container_name}')
    try:
        build_environment_startup_cmd = [
            'docker', 'run', '-di',
            '--name', build_environment_container_name,
            '-v', f'{host_working_dir}:{container_working_directory}',
            '-v', f'{host_install_scripts_dir}:{container_install_scripts_dir}',
            packaging_config.builder_image
        ]
        subprocess.run(build_environment_startup_cmd, check=True)

        container_exec_prefix = [
            'docker', 'exec', '-it',
            '-e', f'WORKING_DIR={container_working_directory}',
            '-e', f'CACHE_DIR={pip_cache_directory}',
            '-e', f'ARTIFACT_NAME={versioned_artifact_name}',
            '-e', f'BUILD_PARALLELISM={build_parallelization}',
            '-w', str(container_working_directory),
            '-u', f'{os.getuid()}:{os.getgid()}',
            build_environment_container_name,
            'bash', '-c'
        ]

        # Run the component installation scripts
        install_cmds = [
            [str(container_install_scripts_dir / 'install-python-component.sh'), 'job-orchestration'],
            [str(container_install_scripts_dir / 'install-python-component.sh'), 'clp-py-utils'],
            [str(container_install_scripts_dir / 'install-python-component.sh'), 'clp-package-utils'],
            [str(container_install_scripts_dir / 'install-python-component.sh'), 'compression-job-handler'],
            [str(container_install_scripts_dir / 'install-core.sh')]
        ]
        for cmd in install_cmds:
            joined_cmd = ' '.join(cmd)
            container_exec_cmd = [*container_exec_prefix, joined_cmd]
            log.info(' '.join(container_exec_cmd))
            subprocess.run(container_exec_cmd, check=True)

        archive_cmd = f'tar -czf {versioned_artifact_name}.tar.gz {versioned_artifact_name}'
        subprocess.run([*container_exec_prefix, archive_cmd], check=True)
    except subprocess.CalledProcessError as ex:
        print(ex.stdout)
        log.error('Failed to build CLP')
    except:
        log.exception('Failed to build CLP')
    finally:
        # Cleanup
        log.info('Cleaning up')
        try:
            subprocess.run(['docker', 'rm', '-f', build_environment_container_name],
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        except:
            pass

        # Verify whether artifact is generated
        artifact_tarball_path = host_working_dir / f'{versioned_artifact_name}.tar.gz'
        if artifact_tarball_path.exists():
            log.info(f'Artifact built successfully: {artifact_tarball_path}')
        else:
            log.error('Artifact build failure')
            return -1

    return 0


if '__main__' == __name__:
    sys.exit(main(sys.argv))
