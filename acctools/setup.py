from distutils.core import setup

setup(
    name="acc-tools",
    version="1.0",
    description="ACC test/development tools Python package",
    author="Alex King",
    packages=['acctools'],
    entry_points={
        'console_scripts': [
            'aarch32=acctools.aarch32:main',
        ]
    },
    install_requires=[
        'unicorn>=1.0.2',
        'capstone>=4.0.2',
        'pyelftools>=0.26'
    ]
)