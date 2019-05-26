#!/usr/bin/env python

from setuptools import setup
from setuptools.extension import Extension

if __name__ == '__main__':
    setup(
        name='alsavolctl',
        version='1.0',
        description='ALSA bindings and monitoring',
        author='Ricard Illa',
        author_email='r.illa.pujagut@gmail.com',
        url='https://github.com/gthar/alsavolctl',
        packages=['alsavolctl'],
        entry_points={
            'console_scripts': ["vol_webui=alsavolctl.vol_webui:main"]},
        py_modules=['alsavolctl.mixer', 'alsavolctl.vol_webui'],
        install_requires=['websockets'],
        ext_modules=[Extension(
            'alsavolctl._mixer',
            ['src/mixer.c'],
            libraries=['asound'])])
