
project('opcua-test', 'cpp', 
    default_options : ['cpp_std=c++11'],
	version : '1.0.0')

open62541dep = dependency('open62541')

server_sources = [ 'server.cpp' ]

open62541_server = executable('server', 
	server_sources, 
	dependencies: [ 
		open62541dep
	]
)

client_sources = [ 'client.cpp' ]

open62541_client = executable('client', 
	client_sources, 
	dependencies: [ 
		open62541dep
	]
)

open62541_client_abrupt = executable('client-abrupt-disconnect', 
	client_sources, 
	dependencies: [ 
		open62541dep
	],
	cpp_args: '-DABRUPT_DISCONNECT'
)

open62541_client_resub = executable('client-resubscribe', 
	client_sources, 
	dependencies: [ 
		open62541dep
	],
	cpp_args: '-DCLIENT_RESUB'
)