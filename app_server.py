#
# RCAS - Remote Console Application Server
# Peter Peterson <peter@saborgato.com>
#
from subprocess import *
from threading import Thread
import sys
import getopt
import websockets
import asyncio

class RingBuffer:
    def __init__(self, size_max):
        self.max = size_max
        self.data = []

    class __Full:
        def append(self, x):
            self.data[self.cur] = x
            self.cur = (self.cur+1) % self.max
        def get(self):
            return self.data[self.cur:]+self.data[:self.cur]

    def append(self,x):
        self.data.append(x)
        if len(self.data) == self.max:
            self.cur = 0
            self.__class__ = self.__Full

    def get(self):
        return self.data

class ConsoleAppServer:
    process = None
    thread = None
    connection_queues = {}
    loop = asyncio.get_event_loop()
    ring = RingBuffer(20)

    def enqueue_output(self, out, connection_queues):
        for line in iter(out.readline, b''):
            if self.process.poll():
                out.close()
                return

            print(line)

            for queue in self.connection_queues.values():
                self.loop.call_soon_threadsafe(queue.put_nowait, line)

            self.ring.append(line)

        out.close()
    
    def __init__(self, app):
        self.process = Popen([f"{app}"], stdin=PIPE, stdout=PIPE, bufsize=1, universal_newlines=True)

        print(f"Process started: {self.process.args}")
        print("Console output starting...")

        self.thread = Thread(target=self.enqueue_output, args=(self.process.stdout, self.connection_queues))
        self.thread.daemon = True
        self.thread.start()

    def close(self):
        self.process.terminate()

    def create_queue(self, websocket):
        queue = asyncio.Queue()
        self.connection_queues[websocket] = queue
        return queue

    def destroy_queue(self, websocket):
        q = self.connection_queues.pop(websocket)
        while True:
            try:
                q.get_nowait()
            except asyncio.QueueEmpty:
                print("queue is drained")
                break

    def broadcast_message(self, message):
        print(message)
        for queue in self.connection_queues.values():
            queue.put_nowait(message)

    def send(self, command):
        print('sending command: ' + command)
        self.broadcast_message(command)
        
        self.ring.append(command)
        self.process.stdin.write(command + '\n')

class ChatServer:
    thread = None
    connection_queues = {}
    loop = asyncio.get_event_loop()
    ring = RingBuffer(20)

    def create_queue(self, websocket):
        queue = asyncio.Queue()
        self.connection_queues[websocket] = queue
        return queue

    def destroy_queue(self, websocket):
        q = self.connection_queues.pop(websocket)
        while True:
            try:
                q.get_nowait()
            except asyncio.QueueEmpty:
                print("chat queue is drained")
                break

    def broadcast_message(self, message):
        for queue in self.connection_queues.values():
            queue.put_nowait(message)

    def send(self, handle, message):
        message = f"[{handle}] {message}"
        self.broadcast_message(message)
        self.ring.append(message)


cas = None
chat = ChatServer()

async def console_command_handler(websocket, path):
    async for message in websocket:
        print(f"received command: {message}")
        cas.send(message)

async def console_log_handler(websocket, path):
    queue = cas.create_queue(websocket)
    while True:
        log = await queue.get()
        queue.task_done()
        await websocket.send(log)

async def incoming_chat_handler(websocket, path, handle):
    async for message in websocket:
        chat.send(handle, message)

async def group_chat_handler(websocket, path):
    queue = chat.create_queue(websocket)
    while True:
        message = await queue.get()
        queue.task_done()
        await websocket.send(message)
        
async def ws_handler(websocket, path):
    if path == '/chat':
        # the chat path
        handle = str(websocket.remote_address[1])

        chat.broadcast_message(f"[server] {handle} has connected")

        for message in chat.ring.get():
            await websocket.send(message)

        incoming_chat_task = asyncio.ensure_future(incoming_chat_handler(websocket, path, handle))
        group_chat_task = asyncio.ensure_future(group_chat_handler(websocket, path))

        done, pending = await asyncio.wait([incoming_chat_task, group_chat_task], return_when=asyncio.FIRST_COMPLETED)

        for task in pending:
            chat.destroy_queue(websocket)
            chat.broadcast_message(f"[server] {handle} has disconnected")
            task.cancel()

    elif path == '/console':
        cas.broadcast_message(f"Client [{websocket.remote_address[1]}] connected from remote IP {websocket.remote_address[0]}")

        await websocket.send(f"Connected to RCAS process: {cas.process.args}")

        await websocket.send(f"Replaying last {cas.ring.max} console entries...")

        for log in cas.ring.get():
            await websocket.send(log)

        command_task = asyncio.ensure_future(console_command_handler(websocket, path))
        log_task = asyncio.ensure_future(console_log_handler(websocket, path))

        done, pending = await asyncio.wait([command_task, log_task], return_when=asyncio.FIRST_COMPLETED)

        for task in pending:
            cas.destroy_queue(websocket)
            cas.broadcast_message(f"Client [{websocket.remote_address[1]}] disconnected")
            task.cancel()

    else:
        await websocket.send("Incorrect path")

def main(argv):
    global cas
    port = 9001
    app = None

    if not (sys.version_info.major == 3 and sys.version_info.minor >= 6):
        print("This application requires Python 3.6 or higher!")
        print("You are using Python {}.{}.".format(sys.version_info.major, sys.version_info.minor))
        sys.exit(1)

    help = \
        'usage: app_server.py [options]\n' \
        'options:\n' \
        '\t-h\t\tShow help.\n' \
        '\t-p --port\tListening port.' \
        '\n' \
        '\t-a --app\tpath to application to remote\n'

    try:
        opts, args = getopt.getopt(argv, "hp:a:", [ "port=", 
                                                    "app=" ])
    except getopt.GetoptError:
        print("Invalid args")
        print(help)
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print(help)
            sys.exit()
        elif opt in ("-p", "--port"):
            port = int(arg)
        elif opt in ("-a", "--app"):
            app = arg

    cas = ConsoleAppServer(app or 'C:\\Users\\peter\\Desktop\\bedrock-server-1.14.60.5\\bedrock_server.exe')

    start_server = websockets.serve(ws_handler, "0.0.0.0", port)

    print(f"Server listening on port {port}")

    asyncio.get_event_loop().run_until_complete(start_server)
    asyncio.get_event_loop().run_forever()

if __name__ == "__main__":
    main(sys.argv[1:])