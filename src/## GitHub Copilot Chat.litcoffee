## GitHub Copilot Chat

- Extension: 0.39.2 (prod)
- VS Code: 1.111.0 (ce099c1ed25d9eb3076c11e4a280f3eb52b4fbeb)
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
- DNS ipv4 Lookup: Error (1 ms): getaddrinfo EAI_AGAIN api.github.com
- DNS ipv6 Lookup: Error (0 ms): getaddrinfo EAI_AGAIN api.github.com
- Proxy URL: None (0 ms)
- Electron fetch: Unavailable
- Node.js https: Error (19 ms): Error: getaddrinfo EAI_AGAIN api.github.com
	at GetAddrInfoReqWrap.onlookupall [as oncomplete] (node:dns:122:26)
- Node.js fetch (configured): Error (23 ms): TypeError: fetch failed
	at node:internal/deps/undici/undici:14902:13
	at process.processTicksAndRejections (node:internal/process/task_queues:105:5)
	at async n._fetch (/home/llne/.vscode-server/extensions/github.copilot-chat-0.39.2/dist/extension.js:5075:5229)
	at async n.fetch (/home/llne/.vscode-server/extensions/github.copilot-chat-0.39.2/dist/extension.js:5075:4541)
	at async u (/home/llne/.vscode-server/extensions/github.copilot-chat-0.39.2/dist/extension.js:5107:186)
	at async Zm._executeContributedCommand (file:///home/llne/.vscode-server/bin/ce099c1ed25d9eb3076c11e4a280f3eb52b4fbeb/out/vs/workbench/api/node/extensionHostProcess.js:494:48672)
  Error: getaddrinfo EAI_AGAIN api.github.com
  	at GetAddrInfoReqWrap.onlookupall [as oncomplete] (node:dns:122:26)

Connecting to https://api.githubcopilot.com/_ping:
- DNS ipv4 Lookup: Error (0 ms): getaddrinfo EAI_AGAIN api.githubcopilot.com
- DNS ipv6 Lookup: Error (1 ms): getaddrinfo EAI_AGAIN api.githubcopilot.com
- Proxy URL: None (1 ms)
- Electron fetch: Unavailable
- Node.js https: Error (24 ms): Error: getaddrinfo EAI_AGAIN api.githubcopilot.com
	at GetAddrInfoReqWrap.onlookupall [as oncomplete] (node:dns:122:26)
- Node.js fetch (configured): Error (23 ms): TypeError: fetch failed
	at node:internal/deps/undici/undici:14902:13
	at process.processTicksAndRejections (node:internal/process/task_queues:105:5)
	at async n._fetch (/home/llne/.vscode-server/extensions/github.copilot-chat-0.39.2/dist/extension.js:5075:5229)
	at async n.fetch (/home/llne/.vscode-server/extensions/github.copilot-chat-0.39.2/dist/extension.js:5075:4541)
	at async u (/home/llne/.vscode-server/extensions/github.copilot-chat-0.39.2/dist/extension.js:5107:186)
	at async Zm._executeContributedCommand (file:///home/llne/.vscode-server/bin/ce099c1ed25d9eb3076c11e4a280f3eb52b4fbeb/out/vs/workbench/api/node/extensionHostProcess.js:494:48672)
  Error: getaddrinfo EAI_AGAIN api.githubcopilot.com
  	at GetAddrInfoReqWrap.onlookupall [as oncomplete] (node:dns:122:26)

Connecting to https://copilot-proxy.githubusercontent.com/_ping:
- DNS ipv4 Lookup: Error (0 ms): getaddrinfo EAI_AGAIN copilot-proxy.githubusercontent.com
- DNS ipv6 Lookup: Error (0 ms): getaddrinfo EAI_AGAIN copilot-proxy.githubusercontent.com
- Proxy URL: None (47 ms)
- Electron fetch: Unavailable
- Node.js https: Error (19 ms): Error: getaddrinfo EAI_AGAIN copilot-proxy.githubusercontent.com
	at GetAddrInfoReqWrap.onlookupall [as oncomplete] (node:dns:122:26)
- Node.js fetch (configured): Error (24 ms): TypeError: fetch failed
	at node:internal/deps/undici/undici:14902:13
	at process.processTicksAndRejections (node:internal/process/task_queues:105:5)
	at async n._fetch (/home/llne/.vscode-server/extensions/github.copilot-chat-0.39.2/dist/extension.js:5075:5229)
	at async n.fetch (/home/llne/.vscode-server/extensions/github.copilot-chat-0.39.2/dist/extension.js:5075:4541)
	at async u (/home/llne/.vscode-server/extensions/github.copilot-chat-0.39.2/dist/extension.js:5107:186)
	at async Zm._executeContributedCommand (file:///home/llne/.vscode-server/bin/ce099c1ed25d9eb3076c11e4a280f3eb52b4fbeb/out/vs/workbench/api/node/extensionHostProcess.js:494:48672)
  Error: getaddrinfo EAI_AGAIN copilot-proxy.githubusercontent.com
  	at GetAddrInfoReqWrap.onlookupall [as oncomplete] (node:dns:122:26)

Connecting to https://mobile.events.data.microsoft.com: Error (23 ms): TypeError: fetch failed
	at node:internal/deps/undici/undici:14902:13
	at process.processTicksAndRejections (node:internal/process/task_queues:105:5)
	at async n._fetch (/home/llne/.vscode-server/extensions/github.copilot-chat-0.39.2/dist/extension.js:5075:5229)
	at async n.fetch (/home/llne/.vscode-server/extensions/github.copilot-chat-0.39.2/dist/extension.js:5075:4541)
	at async u (/home/llne/.vscode-server/extensions/github.copilot-chat-0.39.2/dist/extension.js:5112:136)
	at async Zm._executeContributedCommand (file:///home/llne/.vscode-server/bin/ce099c1ed25d9eb3076c11e4a280f3eb52b4fbeb/out/vs/workbench/api/node/extensionHostProcess.js:494:48672)
  Error: getaddrinfo EAI_AGAIN mobile.events.data.microsoft.com
  	at GetAddrInfoReqWrap.onlookupall [as oncomplete] (node:dns:122:26)
Connecting to https://dc.services.visualstudio.com: Error (69 ms): TypeError: fetch failed
	at node:internal/deps/undici/undici:14902:13
	at process.processTicksAndRejections (node:internal/process/task_queues:105:5)
	at async n._fetch (/home/llne/.vscode-server/extensions/github.copilot-chat-0.39.2/dist/extension.js:5075:5229)
	at async n.fetch (/home/llne/.vscode-server/extensions/github.copilot-chat-0.39.2/dist/extension.js:5075:4541)
	at async u (/home/llne/.vscode-server/extensions/github.copilot-chat-0.39.2/dist/extension.js:5112:136)
	at async Zm._executeContributedCommand (file:///home/llne/.vscode-server/bin/ce099c1ed25d9eb3076c11e4a280f3eb52b4fbeb/out/vs/workbench/api/node/extensionHostProcess.js:494:48672)
  Error: getaddrinfo EAI_AGAIN dc.services.visualstudio.com
  	at GetAddrInfoReqWrap.onlookupall [as oncomplete] (node:dns:122:26)
Connecting to https://copilot-telemetry.githubusercontent.com/_ping: Error (19 ms): Error: getaddrinfo EAI_AGAIN copilot-telemetry.githubusercontent.com
	at GetAddrInfoReqWrap.onlookupall [as oncomplete] (node:dns:122:26)
Connecting to https://copilot-telemetry.githubusercontent.com/_ping: Error (19 ms): Error: getaddrinfo EAI_AGAIN copilot-telemetry.githubusercontent.com
	at GetAddrInfoReqWrap.onlookupall [as oncomplete] (node:dns:122:26)
Connecting to https://default.exp-tas.com: Error (18 ms): Error: getaddrinfo EAI_AGAIN default.exp-tas.com
	at GetAddrInfoReqWrap.onlookupall [as oncomplete] (node:dns:122:26)

Number of system certificates: 432

## Documentation

In corporate networks: [Troubleshooting firewall settings for GitHub Copilot](https://docs.github.com/en/copilot/troubleshooting-github-copilot/troubleshooting-firewall-settings-for-github-copilot).