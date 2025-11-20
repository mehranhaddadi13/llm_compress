from setuptools import setup, Extension

setup(
    name = 'pyTawa',
    version = '1.1',
    description = 'A Python wrapper for Tawa',
    url = None,
    author = 'William Teahan',
    author_email = 'w.j.teahan@bangor.ac.uk',
    package_data={'pyTawa': ['pyTawa.so']}
)
