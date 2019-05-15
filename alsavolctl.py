from _alsavolctl import Mixer


def vol_scaler(volume, volume_range, method="cube", n=3):
    """
    Scale the volume to the range 0-100
    """
    pmin, pmax = volume_range
    ran = pmax - pmin
    scaled = (volume - pmin) / ran
    if method == "cube":
        scaled = scaled ** n
    return round(scaled * 100)


def vol_unscaler(volume, volume_range, method="cube", n=3):
    """
    Bring back a value scaled as a percentage to a physical volume
    """
    pmin, pmax = volume_range
    ran = pmax - pmin
    scaled = volume / 100
    if method == "cube":
        scaled = scaled ** (1/n)
    return round(scaled * ran + pmin)
