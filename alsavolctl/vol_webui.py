#!/usr/bin/env python3

"""
Server part of the vol_webui. Communicate with the client through WebSockets
"""

import argparse
import asyncio
import functools
import json

import websockets

from alsavolctl.mixer import Mixer


async def broadcast(connected, msg):
    """
    Given a list of connected clients and a message, send the message to all
    of them
    """
    for socket in connected:
        await socket.send(msg)


def mk_msg(val_type, value):
    """
    Given a type of message (vol/mute) and its value, create a JSON message
    for it
    """
    return json.dumps({'type': val_type, 'value': value})


async def producer(mixer_ctl, connected):
    """
    Given a mixer, a card and a list of connected clients, watch for changes in
    volume and mute state and broadcast any changes to all clients
    """
    print("starting monitor loop")

    async for msg in mixer_ctl.monitor_async():

        for param, val in msg.items():

            if param == "volume":
                volume = mixer_ctl.getvolume_scaled()
                print("vol changed to {} ({})".format(val, volume))
                await broadcast(connected, mk_msg('volume', volume))

            if param == "switch":
                mute = not val
                print("switch changed to {} (mute={})".format(val, mute))
                await broadcast(connected, mk_msg('mute', mute))


async def handler(websocket, _, connected, mixer_ctl):
    """
    WebSocket handler. Watch for changes in the clients and update the volume
    accordingly
    """
    connected.add(websocket)
    volume = mixer_ctl.getvolume_scaled()
    mute = not mixer_ctl.getswitch()
    try:
        await websocket.send(mk_msg('volume', volume))
        await websocket.send(mk_msg('mute', mute))

        async for msg in websocket:
            data = json.loads(msg)

            if data['type'] == 'volume':
                mixer_ctl.setvolume_unscaled(data['value'])

            elif data['type'] == 'mute':
                switch = not mixer_ctl.getswitch()
                mixer_ctl.setswitch(switch)

            else:
                print("unsupported event: {}", data)

    finally:
        connected.remove(websocket)


def start_server(mixer_info, address):
    """
    Start the WebSockets server
    """
    connected = set()
    mixer_ctl = Mixer(
        mixer_info['card'],
        mixer_info['device'],
        mixer_info['mixer'])

    loop = asyncio.get_event_loop()
    loop.create_task(producer(mixer_ctl, connected))
    loop.run_until_complete(websockets.serve(
        functools.partial(
            handler,
            connected=connected,
            mixer_ctl=mixer_ctl),
        address['host'],
        address['port']))
    loop.run_forever()


def main():
    """
    Parse the arguments and to the thing
    """
    parser = argparse.ArgumentParser(description="Volume Web UI")
    parser.add_argument("--host", type=str, default='localhost')
    parser.add_argument("--card", type=str, default='hw:0')
    parser.add_argument("--device", type=str, default='default')
    parser.add_argument("--mixer", type=str, default='Master')
    parser.add_argument("--port", type=int, default=6789)
    args = parser.parse_args()
    mixer_info = {
        'card': args.card,
        'device': args.device,
        'mixer': args.mixer}
    address = {'host': args.host, 'port': args.port}
    start_server(mixer_info, address)
