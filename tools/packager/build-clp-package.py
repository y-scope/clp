import logging
import pathlib
import shutil
import subprocess
import sys
import uuid
from concurrent.futures import ProcessPoolExecutor

import psutil
import yaml
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


def clone_and_checkout(component_name, component, working_dir):
    git_clone_command = ['git', 'clone', '-b', component['branch'], '--depth', '1', component['url'], component_name]
    subprocess.run(git_clone_command, cwd=working_dir, check=True)


def main(argv):
    try:
        check_dependencies()
    except EnvironmentError:
        log.error('Unmet build dependency')
        return

    # Parse yaml file
    with open('build-clp-package.yaml') as config_file:
        config = yaml.load(config_file, Loader=SafeLoader)

    # Limit maximum build parallelization degree to minimize chance of running out of RAM
    # Minimum 2GB per core to ensure successful compilation
    if int(config['build-parallelism']) == 0:
        build_parallelization = min(int(psutil.virtual_memory().total / (2 * 1024 * 1024 * 1024)), psutil.cpu_count())
    elif int(config['build-parallelism']) > 0:
        build_parallelization = int(config['build-parallelism'])
    else:
        log.error(f'Unsupported parallelization: {str(config["build-parallelism"])}')
        return

    # Infer install scripts directory
    host_install_scripts_dir = pathlib.Path(__file__).parent.resolve() / 'install-scripts'
    container_install_scripts_dir = pathlib.PurePath('/tmp/install-scripts')

    # Remove existing out directory to ensure clean state prior to cloning directories
    host_working_dir = pathlib.Path(config['working-dir'])
    try:
        shutil.rmtree(host_working_dir)
    except FileNotFoundError:
        pass
    except Exception as ex:
        log.error(ex)
        log.error(f'Failed to clean up working directory: {str(host_working_dir)}')
        return

    host_working_dir.mkdir(parents=True, exist_ok=True)
    host_working_dir = host_working_dir.resolve()
    container_working_directory = pathlib.PurePath('/tmp/out')

    # # Clone and check out in parallel
    with ProcessPoolExecutor() as executor:
        for component_name, component in config['components'].items():
            executor.submit(clone_and_checkout, component_name, component, host_working_dir)

    # # Create a copy of the base package & clean up misc files
    artifact_dir = (host_working_dir / config['artifact-name']).resolve()
    shutil.copytree(host_working_dir / 'clp-package-base', artifact_dir)
    shutil.rmtree(artifact_dir / '.git', ignore_errors=True)

    # Start build environment container
    build_environment_container_name = f'clp-builder-{str(uuid.uuid4())}'

    try:
        build_environment_startup_cmd = [
            'docker', 'run', '-di',
            '--name', build_environment_container_name,
            '-v', f'{str(host_working_dir)}:{str(container_working_directory)}',
            '-v', f'{str(host_install_scripts_dir)}:{str(container_install_scripts_dir)}',
            config['builder-dockerhub-image']
        ]
        subprocess.run(build_environment_startup_cmd, check=True)

        container_exec_prefix = [
            'docker', 'exec', '-it',
            '-e', f'WORKING_DIR={str(container_working_directory)}',
            '-e', f'INSTALL_SCRIPT_DIR={str(container_install_scripts_dir)}',
            '-e', f'ARTIFACT_NAME={config["artifact-name"]}',
            '-e', f'BUILD_PARALLELISM={str(build_parallelization)}',
            '-w', str(container_working_directory),
            build_environment_container_name
        ]

        # Invoke install scripts inside containers
        scripts = [
            'install-compression-job-handler.sh',
            'install-job-orchestration.sh',
            'install-clp-py-utils.sh',
            'install-clp-core.sh'
        ]
        subprocess.run(container_exec_prefix +
                       ['chmod', 'ugo+x', '-R', f'{str(container_install_scripts_dir)}'], check=True)
        for script in scripts:
            container_exec_cmd = container_exec_prefix + [str(container_install_scripts_dir / script)]
            subprocess.run(container_exec_cmd, check=True)

        # Invoke left-over commands
        cmds = [
            'pip3 install celery==5.1.2',
            f'cp /usr/local/bin/celery {config["artifact-name"]}/bin/',
            f'chmod -R ugo+rw {str(container_working_directory)}/clp-core',
            f'chmod -R ugo+x {config["artifact-name"]}',
            f'chmod -R ugo+x {config["artifact-name"]}/sbin',
            f'chmod -R ugo+rw {config["artifact-name"]}',
            f'tar -czf {config["artifact-name"]}.tar.gz {config["artifact-name"]}',
            f'chmod ugo+rw {config["artifact-name"]}.tar.gz'
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
        artifact_tarball_path = artifact_dir.parent / f'{config["artifact-name"]}.tar.gz'
        if artifact_tarball_path.exists():
            log.info(f'Artifact built successfully: {str(artifact_tarball_path)}')
        else:
            log.error('Artifact build failure')


if '__main__' == __name__:
    main(sys.argv)
