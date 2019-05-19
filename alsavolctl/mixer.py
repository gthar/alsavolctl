import asyncio
from alsavolctl._mixer import Mixer as _Mixer


class Mixer():
    """
    Wrapper of _mixer.Mixer. This is basically to be able to make a
    subtype of that one without the hassle of doing it properly (which implies
    some work in the C code). Will probably do it the proper way in the future.
    """

    def __init__(self, card, device, mixer):
        self._mixer = _Mixer(card, device, mixer)


    def getvolume_scaled(self, method="cube", n=3):
        pmin, pmax = self._mixer.getrange()
        ran = pmax - pmin
        scaled = (self._mixer.getvolume() - pmin) / ran
        if method == "cube":
            scaled = scaled ** n
        return round(scaled * 100)


    def setvolume_unscaled(self, vol, method="cube", n=3):
        pmin, pmax = self._mixer.getrange()
        ran = pmax - pmin
        scaled = vol / 100

        if method == "cube":
            scaled = scaled ** (1/n)
        val = round(scaled * ran + pmin)

        self._mixer.setvolume(val)


    def getswitch(self):
        return self._mixer.getswitch()


    def setswitch(self, switch):
        self._mixer.setswitch(switch)


    def monitor(self):
        while True:
            yield self._mixer.wait_for_event()


    async def monitor_async(self):
        loop = asyncio.get_event_loop()
        while True:
            result = await loop.run_in_executor(
                None,
                self._mixer.wait_for_event)
            yield result
