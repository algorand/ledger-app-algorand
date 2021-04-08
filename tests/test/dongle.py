import time
import threading
import socket
import json
import logging
import re
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
        self.messages_seen = []

    def exchange(self, apdu, timeout=20000):
        return bytes(self.dongle.exchange(apdu, timeout))

    def close(self):
        self.dongle.close()

    def append_dived_messages(self):
        new_labels = []
        i = 0 
        while i < len(self.messages_seen):
            if len(re.findall(r'\([0-9]+\/[0-9]\)',self.messages_seen[i])) == 0 :
                if self.messages_seen[i] != 'application' and self.messages_seen[i] != 'is ready':
                    new_labels.append([self.messages_seen[i],""])
                i += 1
                continue
            else:
                title = self.messages_seen[i][:self.messages_seen[i].find(" (")]
                number_of_chunks_exp = int(self.messages_seen[i][self.messages_seen[i].find("/")+1:self.messages_seen[i].find(")")])
                element = [title,""]
                j = i +1 
                while j < i + number_of_chunks_exp*2:
                    element = [title,element[1]+self.messages_seen[j]]
                    j+=2
                i = i + number_of_chunks_exp*2
                new_labels.append(element)
                

        return new_labels


    def get_messages(self):
        return self.append_dived_messages()

    @contextmanager
    def screen_event_handler(self, handler):
        def do_handle_events(_handler, _fd):
            buttons = Buttons(self.button_port)
            try:
                self.messages_seen
                for line in _fd:
                    if callable(handler):
                        self.messages_seen = handler(json.loads(line.strip('\n')), buttons, self.messages_seen)
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
        s.shutdown(socket.SHUT_RDWR)
        fd.close()
        t.join()

        logger.info('Closing connection to 127.0.0.1:%d' % self.automation_port)
        s.close()
        self.messages_seen = []


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
