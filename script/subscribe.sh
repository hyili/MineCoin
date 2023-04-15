#!/bin/bash

./build/boost-websocket-client max-stream.maicoin.com 443 '{"action":"sub", "subscriptions":[{"channel":"book", "market": "btctwd", "depth": 1}], "id": "client1"}'
