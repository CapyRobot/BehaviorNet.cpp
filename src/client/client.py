import http.server
import socketserver

PORT = 8090

class Robot:

    action_in_exec = ""
    action_in_exec_done = False

    def action_command(self, action):
        if self.action_in_exec == action:
            if self.action_in_exec_done:
                self.action_in_exec = ""
                self.action_in_exec_done = False
                return b"success"
            else:
                return b"in_progress"
        elif self.action_in_exec != "": # executing something else
            return b"failure"
        else:
            self.action_in_exec = action
            self.action_in_exec_done = False
            return b"in_progress"

    def finish_action(self):
        self.action_in_exec_done = True


robot = Robot()

class MyRequestHandler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/action/c':
            r = robot.action_command(self.path)
            print("robot.action_command(self.path)", r)

            self.send_response(200)
            self.send_header("Content-type", "text/html")
            self.end_headers()
            self.wfile.write(r)
        elif self.path == '/action_done':
            robot.finish_action()
            self.send_response(200)
            self.send_header("Content-type", "text/html")
            self.end_headers()
            self.wfile.write(b"ok\n")
        else:
            self.send_response(404)
            self.end_headers()

socketserver.TCPServer.allow_reuse_address = True
with socketserver.TCPServer(("", PORT), MyRequestHandler) as httpd:
    print("serving at port", PORT)
    httpd.serve_forever()
