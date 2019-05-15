#!/usr/bin/env python

from setuptools import setup
from setuptools.extension import Extension

if __name__ == '__main__':
    setup(
        name = 'alsavolctl',
        version = '1.0',
        description = 'ALSA bindings and monitoring',
        author = 'Ricard Illa',
        author_email = 'r.illa.pujagut@gmail.com',
        url = 'https://github.com/gthar/alsavolctl',
        py_modules = ['alsavolctl'],
        ext_modules=[
            Extension('_alsavolctl', ['mixer_class.c'], libraries=['asound'])]
    )
