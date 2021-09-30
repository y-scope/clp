import argparse
import logging
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
from yaml.loader import SafeLoader

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
        currently_supported_component_names = \
            ['package-base', 'compression-job-handler', 'job-orchestration', 'clp-py-utils', 'core']
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
    builder_dockerhub_image: str
    components: typing.List[ClpComponent]


def check_dependencies():
    try:
        subprocess.run('command -v git', shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=True)
    except subprocess.CalledProcessError as ex:
        log.error('Git is not installed on the path.')
        raise EnvironmentError

    try:
        subprocess.run('command -v docker', shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=True)
        subprocess.run(['docker', 'ps'], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=True)
    except subprocess.CalledProcessError as ex:
        log.error('Docker is not installed on the path or does not have permission to run without sudo.')
        raise EnvironmentError


def clone_and_checkout(component: ClpComponent, working_dir: pathlib.Path):
    if component.branch:
        subprocess.run(['git', 'clone', '-b', component.branch, '--depth', '1', component.url, component.name],
                       cwd=working_dir, check=True)
    elif component.commit:
        subprocess.run(['git', 'clone', component.url, component.name], cwd=working_dir, check=True)
        subprocess.run(['git', 'checkout', component.commit], cwd=working_dir / component.name, check=True)


def main(argv):
    args_parser = argparse.ArgumentParser(description='CLP package builder')
    args_parser.add_argument('--config', '-c', required=True, help='CLP configuration file.')

    try:
        check_dependencies()
    except EnvironmentError:
        log.error('Unmet build dependency')
        return

    # Parse yaml file
    with open('../../config/build-clp-package.yaml') as config_file:
        try:
            packaging_config = PackagingConfig.parse_obj(yaml.load(config_file, Loader=SafeLoader))
        except Exception as ex:
            log.error(ex)
            return -1

    # Limit maximum build parallelization degree to minimize chance of running out of RAM
    # Minimum 2GB per core to ensure successful compilation
    if int(packaging_config.build_parallelism) == 0:
        build_parallelization = min(int(psutil.virtual_memory().total / (2 * 1024 * 1024 * 1024)), psutil.cpu_count())
    elif int(packaging_config.build_parallelism) > 0:
        build_parallelization = int(packaging_config.build_parallelism)
    else:
        log.error(f'Unsupported parallelization: {str(packaging_config.build_parallelism)}')
        return

    # Infer install scripts directory
    host_install_scripts_dir = pathlib.Path(__file__).parent.resolve() / 'install-scripts'
    container_install_scripts_dir = pathlib.PurePath('/tmp/install-scripts')

    # Remove existing out directory to ensure clean state prior to cloning directories
    host_working_dir = pathlib.Path(packaging_config.working_dir).resolve()
    try:
        shutil.rmtree(host_working_dir)
    except FileNotFoundError:
        pass
    except Exception as ex:
        log.error(ex)
        log.error(f'Failed to clean up working directory: {str(host_working_dir)}')
        return

    host_working_dir.mkdir(parents=True, exist_ok=True)
    container_working_directory = pathlib.PurePath('/tmp/out')
    versioned_artifact_name = f'{packaging_config.artifact_name}-{packaging_config.arch}-v{packaging_config.version}'
    artifact_dir = (host_working_dir / versioned_artifact_name).resolve()

    # Download or copy source code to build working directory
    with ProcessPoolExecutor() as executor:
        for component in packaging_config.components:
            if 'git' == component.type:
                # For "git" type components, clone and checkout
                executor.submit(clone_and_checkout, component, host_working_dir)
            elif 'local' == component.type:
                # For "local" type components, copy
                shutil.copytree(f'../../components/{component.name}', host_working_dir / component.name)

    # For CLP core, we must download all the dependencies in addition to cloning/copying the repository
    log.info('Cloning clp core dependencies')
    subprocess.run(['./download-all.sh'], cwd=host_working_dir / 'core' / 'tools' / 'scripts' / 'deps-download')

    # Make a copy of package-base and name it as the {artifact_name}-{version}
    shutil.copytree(host_working_dir / 'package-base', artifact_dir)
    shutil.rmtree(artifact_dir / 'package-base' / '.git', ignore_errors=True)

    # Start build environment container
    build_environment_container_name = f'clp-builder-{str(uuid.uuid4())}'
    log.info(f'Starting build environment container {build_environment_container_name}')
    try:
        build_environment_startup_cmd = [
            'docker', 'run', '-di',
            '--name', build_environment_container_name,
            '-v', f'{str(host_working_dir)}:{str(container_working_directory)}',
            '-v', f'{str(host_install_scripts_dir)}:{str(container_install_scripts_dir)}',
            packaging_config.builder_dockerhub_image
        ]
        subprocess.run(build_environment_startup_cmd, check=True)

        container_exec_prefix = [
            'docker', 'exec', '-it',
            '-e', f'WORKING_DIR={str(container_working_directory)}',
            '-e', f'INSTALL_SCRIPT_DIR={str(container_install_scripts_dir)}',
            '-e', f'ARTIFACT_NAME={versioned_artifact_name}',
            '-e', f'BUILD_PARALLELISM={str(build_parallelization)}',
            '-w', str(container_working_directory),
            build_environment_container_name
        ]

        # Invoke install scripts inside containers
        scripts = [
            'install-compression-job-handler.sh',
            'install-job-orchestration.sh',
            'install-clp-py-utils.sh',
            'install-core.sh'
        ]
        subprocess.run(container_exec_prefix +
                       ['chmod', 'ugo+x', '-R', f'{str(container_install_scripts_dir)}'], check=True)
        for script in scripts:
            container_exec_cmd = container_exec_prefix + [str(container_install_scripts_dir / script)]
            log.info(' '.join(container_exec_cmd))
            subprocess.run(container_exec_cmd, check=True)

        # Invoke left-over commands
        cmds = [
            'pip3 install celery==5.1.2',
            f'cp /usr/local/bin/celery {versioned_artifact_name}/bin/',
            f'chmod -R ugo+rw {str(container_working_directory)}/core',
            f'chmod -R ugo+x {versioned_artifact_name}',
            f'chmod -R ugo+x {versioned_artifact_name}/sbin',
            f'chmod -R ugo+rw {versioned_artifact_name}',
            f'tar -czf {versioned_artifact_name}.tar.gz {versioned_artifact_name}',
            f'chmod ugo+rw {versioned_artifact_name}.tar.gz'
        ]
        for cmd in cmds:
            container_exec_cmd = container_exec_prefix + cmd.split()
            subprocess.run(container_exec_cmd, check=True)
    except subprocess.CalledProcessError as ex:
        print(ex.stdout)
        log.error('Failed to build CLP')
    except Exception as ex:
        log.error(str(ex))
        log.error('Failed to build CLP')
    finally:
        # Cleanup
        log.info('Cleaning up')
        try:
            subprocess.run(['docker', 'rm', '-f', build_environment_container_name],
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        except Exception:
            pass

        # Verify whether artifact is generated
        artifact_tarball_path = host_working_dir / f'{versioned_artifact_name}.tar.gz'
        if artifact_tarball_path.exists():
            log.info(f'Artifact built successfully: {str(artifact_tarball_path)}')
        else:
            log.error('Artifact build failure')


if '__main__' == __name__:
    main(sys.argv)
