# -*- coding: utf-8 -*-
from os.path import join


EXPORT_LIST = (
    {
        'lang': 'cpp',
        'proto_path': join('..', 'Source', 'Server', 'common'),
        'header_path': join('..', 'Source', 'Server', 'common'),
        'target': 'server',
    },
    {
        'lang': 'cpp',
        'proto_path': join('..', 'Source', 'Server', 'game', 'src'),
        'header_path': join('..', 'Source', 'Server', 'game', 'src'),
        'target': 'server',
    },
    {
        'lang': 'cpp',
        'proto_path': join('..', 'Source', 'Server', 'db', 'src'),
        'header_path': join('..', 'Source', 'Server', 'db', 'src'),
        'target': 'server',
    },
    {
        'lang': 'cpp',
        'proto_path': join('..', 'Binary', 'Client', 'UserInterface'),
        'header_path': join('..', 'Binary', 'Client', 'UserInterface'),
        'target': 'client',
    },
)
