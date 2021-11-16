# Wesnoth Proxy
Referenced in https://gamehacking.academy/lesson/6/5.

An example proxy for Wesnoth 1.14.9 that allows interception and modification of traffic from a Wesnoth game client to a Wesnoth server. In this case, any time the proxy sees the chat message \wave, it will send an additional chat message saying Hello!.

The majority of the code is based on the Winsock example provided by Microsoft: https://docs.microsoft.com/en-us/windows/win32/winsock/complete-client-code and https://docs.microsoft.com/en-us/windows/win32/winsock/complete-server-code
