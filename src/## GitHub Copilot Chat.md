## GitHub Copilot Chat

- Extension: 0.43.0 (prod)
- VS Code: 1.115.0 (41dd792b5e652393e7787322889ed5fdc58bd75b)
- OS: linux 6.6.87.2-microsoft-standard-WSL2 x64
- Remote Name: wsl
- Extension Kind: Workspace
- GitHub Account: LLNE-Collab

## Network

User Settings:
```json
  "http.systemCertificatesNode": true,
  "github.copilot.advanced.debug.useElectronFetcher": true,
  "github.copilot.advanced.debug.useNodeFetcher": false,
  "github.copilot.advanced.debug.useNodeFetchFetcher": true
```

Connecting to https://api.github.com:
- DNS ipv4 Lookup: Error (2 ms): getaddrinfo EAI_AGAIN api.github.com
- DNS ipv6 Lookup: Error (0 ms): getaddrinfo EAI_AGAIN api.github.com
- Proxy URL: None (1 ms)
- Electron fetch: Unavailable
- Node.js https: Error (23 ms): Error: getaddrinfo EAI_AGAIN api.github.com
	at GetAddrInfoReqWrap.onlookupall [as oncomplete] (node:dns:122:26)
- Node.js fetch (configured): Error (22 ms): TypeError: fetch failed
	at node:internal/deps/undici/undici:14902:13
	at process.processTicksAndRejections (node:internal/process/task_queues:103:5)
	at async t._fetch (/home/llne/.vscode-server/extensions/github.copilot-chat-0.43.0/dist/extension.js:5293:5228)
	at async t.fetch (/home/llne/.vscode-server/extensions/github.copilot-chat-0.43.0/dist/extension.js:5293:4540)
	at async u (/home/llne/.vscode-server/extensions/github.copilot-chat-0.43.0/dist/extension.js:5325:186)
	at async Sg._executeContributedCommand (file:///home/llne/.vscode-server/bin/41dd792b5e652393e7787322889ed5fdc58bd75b/out/vs/workbench/api/node/extensionHostProcess.js:501:48675)
  Error: getaddrinfo EAI_AGAIN api.github.com
  	at GetAddrInfoReqWrap.onlookupall [as oncomplete] (node:dns:122:26)

Connecting to https://api.githubcopilot.com/_ping:
- DNS ipv4 Lookup: Error (1 ms): getaddrinfo EAI_AGAIN api.githubcopilot.com
- DNS ipv6 Lookup: Error (1 ms): getaddrinfo EAI_AGAIN api.githubcopilot.com
- Proxy URL: None (0 ms)
- Electron fetch: Unavailable
- Node.js https: Error (18 ms): Error: getaddrinfo EAI_AGAIN api.githubcopilot.com
	at GetAddrInfoReqWrap.onlookupall [as oncomplete] (node:dns:122:26)
- Node.js fetch (configured): Error (23 ms): TypeError: fetch failed
	at node:internal/deps/undici/undici:14902:13
	at process.processTicksAndRejections (node:internal/process/task_queues:103:5)
	at async t._fetch (/home/llne/.vscode-server/extensions/github.copilot-chat-0.43.0/dist/extension.js:5293:5228)
	at async t.fetch (/home/llne/.vscode-server/extensions/github.copilot-chat-0.43.0/dist/extension.js:5293:4540)
	at async u (/home/llne/.vscode-server/extensions/github.copilot-chat-0.43.0/dist/extension.js:5325:186)
	at async Sg._executeContributedCommand (file:///home/llne/.vscode-server/bin/41dd792b5e652393e7787322889ed5fdc58bd75b/out/vs/workbench/api/node/extensionHostProcess.js:501:48675)
  Error: getaddrinfo EAI_AGAIN api.githubcopilot.com
  	at GetAddrInfoReqWrap.onlookupall [as oncomplete] (node:dns:122:26)

Connecting to https://copilot-proxy.githubusercontent.com/_ping:
- DNS ipv4 Lookup: Error (3 ms): getaddrinfo EAI_AGAIN copilot-proxy.githubusercontent.com
- DNS ipv6 Lookup: Error (1 ms): getaddrinfo EAI_AGAIN copilot-proxy.githubusercontent.com
- Proxy URL: None (46 ms)
- Electron fetch: Unavailable
- Node.js https: Error (28 ms): Error: getaddrinfo EAI_AGAIN copilot-proxy.githubusercontent.com
	at GetAddrInfoReqWrap.onlookupall [as oncomplete] (node:dns:122:26)
- Node.js fetch (configured): Error (23 ms): TypeError: fetch failed
	at node:internal/deps/undici/undici:14902:13
	at process.processTicksAndRejections (node:internal/process/task_queues:103:5)
	at async t._fetch (/home/llne/.vscode-server/extensions/github.copilot-chat-0.43.0/dist/extension.js:5293:5228)
	at async t.fetch (/home/llne/.vscode-server/extensions/github.copilot-chat-0.43.0/dist/extension.js:5293:4540)
	at async u (/home/llne/.vscode-server/extensions/github.copilot-chat-0.43.0/dist/extension.js:5325:186)
	at async Sg._executeContributedCommand (file:///home/llne/.vscode-server/bin/41dd792b5e652393e7787322889ed5fdc58bd75b/out/vs/workbench/api/node/extensionHostProcess.js:501:48675)
  Error: getaddrinfo EAI_AGAIN copilot-proxy.githubusercontent.com
  	at GetAddrInfoReqWrap.onlookupall [as oncomplete] (node:dns:122:26)

Connecting to https://mobile.events.data.microsoft.com: Error (23 ms): TypeError: fetch failed
	at node:internal/deps/undici/undici:14902:13
	at process.processTicksAndRejections (node:internal/process/task_queues:103:5)
	at async t._fetch (/home/llne/.vscode-server/extensions/github.copilot-chat-0.43.0/dist/extension.js:5293:5228)
	at async t.fetch (/home/llne/.vscode-server/extensions/github.copilot-chat-0.43.0/dist/extension.js:5293:4540)
	at async u (/home/llne/.vscode-server/extensions/github.copilot-chat-0.43.0/dist/extension.js:5330:137)
	at async Sg._executeContributedCommand (file:///home/llne/.vscode-server/bin/41dd792b5e652393e7787322889ed5fdc58bd75b/out/vs/workbench/api/node/extensionHostProcess.js:501:48675)
  Error: getaddrinfo EAI_AGAIN mobile.events.data.microsoft.com
  	at GetAddrInfoReqWrap.onlookupall [as oncomplete] (node:dns:122:26)
Connecting to https://dc.services.visualstudio.com: Error (69 ms): TypeError: fetch failed
	at node:internal/deps/undici/undici:14902:13
	at process.processTicksAndRejections (node:internal/process/task_queues:103:5)
	at async t._fetch (/home/llne/.vscode-server/extensions/github.copilot-chat-0.43.0/dist/extension.js:5293:5228)
	at async t.fetch (/home/llne/.vscode-server/extensions/github.copilot-chat-0.43.0/dist/extension.js:5293:4540)
	at async u (/home/llne/.vscode-server/extensions/github.copilot-chat-0.43.0/dist/extension.js:5330:137)
	at async Sg._executeContributedCommand (file:///home/llne/.vscode-server/bin/41dd792b5e652393e7787322889ed5fdc58bd75b/out/vs/workbench/api/node/extensionHostProcess.js:501:48675)
  Error: getaddrinfo EAI_AGAIN dc.services.visualstudio.com
  	at GetAddrInfoReqWrap.onlookupall [as oncomplete] (node:dns:122:26)
Connecting to https://copilot-telemetry.githubusercontent.com/_ping: Error (22 ms): Error: getaddrinfo EAI_AGAIN copilot-telemetry.githubusercontent.com
	at GetAddrInfoReqWrap.onlookupall [as oncomplete] (node:dns:122:26)
Connecting to https://copilot-telemetry.githubusercontent.com/_ping: Error (19 ms): Error: getaddrinfo EAI_AGAIN copilot-telemetry.githubusercontent.com
	at GetAddrInfoReqWrap.onlookupall [as oncomplete] (node:dns:122:26)
Connecting to https://default.exp-tas.com: Error (18 ms): Error: getaddrinfo EAI_AGAIN default.exp-tas.com
	at GetAddrInfoReqWrap.onlookupall [as oncomplete] (node:dns:122:26)

Number of system certificates: 432

## Documentation

In corporate networks: [Troubleshooting firewall settings for GitHub Copilot](https://docs.github.com/en/copilot/troubleshooting-github-copilot/troubleshooting-firewall-settings-for-github-copilot).