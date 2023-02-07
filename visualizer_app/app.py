
# Copyright (C) 2023 Eduardo Rocha
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
# documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

import requests
import argparse
import time


def parse_cli_args():
    parser = argparse.ArgumentParser()

    # mandatory args
    parser.add_argument("bnet_host", help="BNet controller host address.")
    parser.add_argument("bnet_port", help="BNet controller port.")

    # optional args
    # parser.add_argument("-v", "--verbosity", help="increase output verbosity", action="store_true")

    return parser.parse_args()


def get_json_from_http_request(req_url):
    response = requests.get(req_url)
    return response.json()


def render_marking(marking):
    print(marking)


if __name__ == "__main__":
    cli_args = parse_cli_args()
    req_url = "http://" + cli_args.bnet_host + ':' + cli_args.bnet_port + "/get_marking"

    while True:
        marking = get_json_from_http_request(req_url)
        render_marking(marking)
        time.sleep(3)
