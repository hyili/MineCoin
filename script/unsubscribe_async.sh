#!/bin/bash

./build/boost-websocket-client-async-ssl max-stream.maicoin.com 443 '{"action":"unsub", "subscriptions":[{"channel":"book", "market": "btctwd", "depth": 1}], "id": "client1"}'
