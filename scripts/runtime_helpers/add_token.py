
""" helper script to add a token to bnet """

import requests
import argparse
import pprint

TOKEN_CONTENT_MAP = {
    "robot": {
        "content_blocks": [
            {
                "key": "robot",
                "content": {
                    "host": "localhost",
                    "port": 8090
                }
            }
        ]
    },
    "task": {
        "content_blocks": [
            {
                "key": "task",
                "content": {
                    "task_id": "<task id>"
                }
            }
        ]
    }
}

CUSTOM_TOKEN_CONTENT = {
    "content_blocks": [
        {
            "key": "",
            "content": {}
        }
    ]
}

parser = argparse.ArgumentParser(description='Helper script to add a token to bnet.')
parser.add_argument('bnet_addrs', type=str, help='bnet address (host:port)')
parser.add_argument('place_id', type=str, help='bnet place id for insertion')
parser.add_argument('-t', '--token_type', type=str, required=True, help='token type (e.g., robot, task)')
parser.add_argument('-s', '--set_content', type=str, nargs='+', help='set content block data. Format: " +\
    "\"{key1}:{value1}\" \"{key2}:{value2}\" ...')
args = parser.parse_args()

url = args.bnet_addrs + "/add_token"
print("Request url:", url, end="\n\n")

if args.token_type in TOKEN_CONTENT_MAP.keys():
    data = TOKEN_CONTENT_MAP[args.token_type]
else:
    data = CUSTOM_TOKEN_CONTENT
    data["content_blocks"][0]["key"] = args.token_type

data["place_id"] = args.place_id

if args.set_content:
    for pair in args.set_content:
        [key, value] = pair.split(":")
        data["content_blocks"][0]["content"][key] = value

print("Request payload:")
pprint.pprint(data)
print()

print("Posting... ", end="")
try:
    response = requests.post(url, json=data)
except:
    print("failed.\n")
    raise
else:
    print("done.\n")
    print("Response:")
    print(response)
