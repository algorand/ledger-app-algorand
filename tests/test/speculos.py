import os.path
import threading
import socket
import atexit
import logging
from time import sleep

import docker

from . import dongle


CommException = dongle.CommException
logger = logging.getLogger('speculos')


class SpeculosContainer:
    """
    `SpeculosContainer` handles running the Bolos App under test within
    the `speculos` Docker` image.

    A `SpeculosContainer` instance is constructed with the Bolos App ELF
    filename passed as `app` argument and with an optional tcp port passed
    as `apdu_port` argument.

    The Docker container mounts the directory of the `app` within the
    container on the `/app` mountpoint and exposes the `apdu_port` as tcp
    port linked to the default Speculos APDU port (9999).

    The `start()` method starts running the container and starts a background
    thread that reads and logs `stdout` and `stderr` output logs from the
    container. Note that speculos is run in `headless` display mode.

    Besides the `connect()` method creates a `ledgerblue` tcp connection to
    the `speculos` process through the `apdu_port` tcp port.
    """

    def __init__(self, app, apdu_port=9999,
                 automation_port=None, button_port=None):
        self.app = app
        self.apdu_port = apdu_port
        self.automation_port = automation_port or (apdu_port + 1)
        self.button_port = button_port or (apdu_port + 2)
        self.docker = docker.from_env().containers
        self.container = None

    def start(self):
        self.container = self._run_speculos_container()
        self.log_handler = self._log_speculos_output(self.container)
        atexit.register(self.stop)
        logger.info("Started docker container: %s (%s)"
                    % (self.container.image, self.container.name))

    def stop(self):
        logger.info("Stopping docker container: %s (%s)..."
                    % (self.container.image, self.container.name))
        self.container.stop()
        self.log_handler.join()

    def connect(self, debug=False):
        if self.container is None:
            raise dongle.CommException("speculos not started yet")
        return dongle.Dongle(self.apdu_port,
                             self.automation_port,
                             self.button_port,
                             debug=debug)

    def _run_speculos_container(self):
        appdir = os.path.abspath(os.path.dirname(self.app))
        args = [
            '--display headless',
            '--model nanos',
            '--log-level button:DEBUG',
            '--sdk 2.1',
            '--apdu-port 9999',
            '--automation-port 10000',
            '--button-port 10001',
            '/app/%s' % os.path.basename(self.app)
        ]
        c = self.docker.create(image='speculos',
                               command=' '.join(args),
                               volumes={appdir: {'bind': '/app', 'mode': 'ro'}},
                               ports={
                                   '9999/tcp': self.apdu_port,
                                   '10000/tcp': self.automation_port,
                                   '10001/tcp': self.button_port,
                               })
        c.start()
        return c


    def _log_speculos_output(self, container):
        # Synchronize on first log output from container
        cv = threading.Condition()
        started = False

        def do_log():
            for log in container.logs(stream=True, follow=True):
                log_line = log.decode('utf-8').strip('\n')
                # the test process might try to connect to speculos
                # when it is not fully loaded. This may cause the process to halt.
                # in order to prevent it, we try to search for a log message that confirm that
                # it is loaded.
                if "speculos launcher" in log_line:
                    nonlocal started
                    if not started:
                        with cv:
                            started = True
                            cv.notify()
                logger.info(log_line)

        t = threading.Thread(target=do_log, daemon=True)
        t.start()
        with cv:
            while not started:
                cv.wait()
        return t



