import time
import threading
import socket
import json
import logging
from contextlib import contextmanager

import traceback

import ledgerblue.commTCP
from ledgerblue.commException import CommException


logger = logging.getLogger('speculos')


class Dongle:
    def __init__(self, apdu_port=9999, automation_port=None, button_port=None,
                 debug=False):
        self.apdu_port = apdu_port
        self.automation_port = automation_port
        self.button_port = button_port
        self.dongle = ledgerblue.commTCP.getDongle(server='127.0.0.1',
                                                   port=self.apdu_port,
                                                   debug=debug)

    def exchange(self, apdu, timeout=20000):
        return bytes(self.dongle.exchange(apdu, timeout))

    def close(self):
        self.dongle.close()

    @contextmanager
    def screen_event_handler(self, handler):
        def do_handle_events(_handler, _fd):
            buttons = Buttons(self.button_port)
            try:
                for line in _fd:
                    if callable(handler):
                        handler(json.loads(line.strip('\n')), buttons)
            except ValueError:
                pass
            except Exception as e:
                logger.error(e)
                for l in traceback.extract_stack():
                    logger.error(l)
            finally:
                buttons.close()

        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect(('127.0.0.1', self.automation_port))
        fd = s.makefile()
        logger.info('Connected to 127.0.0.1:%d' % self.automation_port)

        t = threading.Thread(target=do_handle_events,
                             args=(handler, fd),
                             daemon=True)
        t.start()
        yield self
        fd.close()
        t.join()

        logger.info('Closing connection to 127.0.0.1:%d' % self.automation_port)
        s.close()


class Buttons:
    LEFT = b'L'
    RIGHT = b'R'
    LEFT_RELEASE = b'l'
    RIGHT_RELEASE = b'r'

    def __init__(self, button_port):
        self.button_port = button_port
        self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.s.connect(('127.0.0.1', button_port))
        logger.info('Buttons: connected to port: %d' % self.button_port)

    def close(self):
        logger.info('Buttons: closing connection to port: %d' % self.button_port)
        self.s.close()

    def press(self, *args):
        for action in args:
            logger.info('Buttons: actions:%s' % action)
            if type(action) == bytes:
                self.s.send(action)
            elif type(action) == str:
                self.s.send(action.encode())
            elif type(action) == int or type(action) == float:
                self.delay(seconds=action)
        return self

    def delay(self, seconds=0.1):
        time.sleep(seconds)
        return self
