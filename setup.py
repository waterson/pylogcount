#!/usr/bin/env python
from distutils.core import setup, Extension

pylogcount_module = Extension(
    "pylogcount",
    sources=["pylogcount.c"],
    libraries=["logcount"])

setup(name="pylogcount", version="0.1", ext_modules=[ pylogcount_module ])
