
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

from flask import Flask
import time
import argparse
import requests

EXEC_TIME_TABLE_SEC = {
    "": 0,
    "goto_fast": 5,
    "goto_medium": 10,
    "goto_slow": 20,
    "charge": 20,
}

BATTERY_CHARGE_DECAY_PER_SEC = 0.005


class Agent:

    def __init__(self) -> None:
        self.last_charge_time = time.time()
        self.task_start_time = 0.0
        self.task_in_exec = ""

        self.__parse_cli_args()
        self.__subscribe_bnet()

        addrs = self.cli_args.agent_host + ':' + self.cli_args.agent_port
        print(f"[Agent::__init__] {self.cli_args.agent_id} configured @ {addrs}")
        addrs = self.cli_args.bnet_host + ':' + self.cli_args.bnet_port
        print(f"[Agent::__init__] BNet subscribed @ {addrs} (place_id: {self.cli_args.bnet_place_id})")

    def __parse_cli_args(self):
        parser = argparse.ArgumentParser()

        # mandatory args
        parser.add_argument("agent_id", help="Id of this agent, e.g., 'AMR-123'.")
        parser.add_argument("agent_host", help="Self host address.")
        parser.add_argument("agent_port", help="Self server port.")
        parser.add_argument("bnet_host", help="BNet controller host address.")
        parser.add_argument("bnet_port", help="BNet controller port.")
        parser.add_argument("bnet_place_id", help="BNet place id used for subscribing, i.e., inserting token at.")

        # optional args
        # parser.add_argument("-v", "--verbosity", help="increase output verbosity", action="store_true")

        self.cli_args = parser.parse_args()

    def __subscribe_bnet(self):
        req_payload = self.__create_subscription_payload()
        req_url = "http://" + self.cli_args.bnet_host + ':' + self.cli_args.bnet_port + "/add_token"
        print(f"[Agent::subscribe_bnet] req_url: {req_url}")
        print(f"[Agent::subscribe_bnet] req_payload: {req_payload}")

        try:
            response = requests.post(req_url, json=req_payload)
        except:
            print("[Agent::subscribe_bnet][ERROR] POST request failure.\n")
            raise

        if response.status_code >= 200 and response.status_code < 300:
            print("[Agent::subscribe_bnet] success.")
        else:
            raise Exception(f"[Agent::subscribe_bnet][ERROR] failure with status code {response.status_code}. " +
            f"Body: {response.text}")

    def __create_subscription_payload(self) -> dict:
        payload = {
            "place_id": self.cli_args.bnet_place_id,
            "content_blocks": [
                {
                    "key": self.cli_args.agent_id,
                    "content": {
                        "host": self.cli_args.agent_host,
                        "port": int(self.cli_args.agent_port)
                    }
                }
            ]
        }
        return payload

    def execute(self, task_id):
        if not self.__task_is_done():
            print(f"[Agent::execute][ERROR] trying to start a new task when another task is still running.")
            return "ACTION_EXEC_STATUS_COMPLETED_ERROR"

        self.task_in_exec = task_id
        self.task_start_time = time.time()
        return "ACTION_EXEC_STATUS_COMPLETED_IN_PROGRESS"

    def get_task_status(self, task_id) -> str:
        if self.task_in_exec != task_id:
            print(f"[Agent::get_status][ERROR] \"{task_id}\" is not running. " +
                  f"Instead, the current task is \"{self.task_in_exec}\".")
            return "ACTION_EXEC_STATUS_COMPLETED_ERROR"

        if self.__task_is_done():
            self.task_in_exec = ""
            return "ACTION_EXEC_STATUS_COMPLETED_SUCCESS"
        else:
            return "ACTION_EXEC_STATUS_COMPLETED_IN_PROGRESS"

    def __task_is_done(self):
        delta = time.time() - self.task_start_time
        return self.task_in_exec == "" or delta > EXEC_TIME_TABLE_SEC[self.task_in_exec]

    def print_state(self):
        if self.__task_is_done():
            print("[Agent::print_state] idle.")
            return "Idle."
        else:
            print("[Agent::print_state] executing " + self.task_in_exec + ".")
            return "Executing " + self.task_in_exec + "."

    def get_battery_charge(self):
        delta = time.time() - self.last_charge_time
        return max(0.0, 1.0 - (BATTERY_CHARGE_DECAY_PER_SEC * delta))


agent = Agent()
app = Flask(__name__)


@app.route('/execute/<task_id>', methods=['GET', 'POST'])
def task_request(task_id):
    return agent.execute(task_id)


@app.route('/get_status/<task_id>', methods=['GET', 'POST'])
def status_request(task_id):
    return agent.get_task_status(task_id)


@app.route('/print_state',  methods=['GET', 'POST'])
def print_state_request():
    return agent.print_state()


@app.route('/battery_charge', methods=['GET'])
def get_battery_charge_request():
    return agent.get_battery_charge()


if __name__ == '__main__':
    app.run(port=agent.cli_args.agent_port)
