/*
    Licensed to the Apache Software Foundation (ASF) under one
    or more contributor license agreements.  See the NOTICE file
    distributed with this work for additional information
    regarding copyright ownership.  The ASF licenses this file
    to you under the Apache License, Version 2.0 (the
    "License"); you may not use this file except in compliance
    with the License.  You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing,
    software distributed under the License is distributed on an
    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
    KIND, either express or implied.  See the License for the
    specific language governing permissions and limitations
    under the License.
*/

import Cordova
import Foundation
import Network

// Keeps the embedded server alive for the app's lifetime.
private var gWZServer: WZLocalServer?
private let kWZPort: UInt16 = 8642

// Minimal loopback HTTP/1.1 static file server. Its whole reason to exist:
// serving the bundled WASM build over http://localhost gives the WebView a
// "secure context" origin that, together with COOP/COEP, becomes
// cross-origin isolated -> SharedArrayBuffer / WASM threads work.
// WKWebView never grants that to the custom app:// scheme.
final class WZLocalServer {
    private let root: URL
    private let port: NWEndpoint.Port
    private var listener: NWListener?
    private let queue = DispatchQueue(label: "wz.localserver", attributes: .concurrent)

    init(root: URL, port: UInt16) {
        self.root = root
        self.port = NWEndpoint.Port(rawValue: port)!
    }

    func start() {
        do {
            let params = NWParameters.tcp
            params.allowLocalEndpointReuse = true
            let l = try NWListener(using: params, on: port)
            l.newConnectionHandler = { [weak self] conn in self?.accept(conn) }
            l.start(queue: queue)
            listener = l
            NSLog("[WZ] local server listening on http://localhost:\(port.rawValue)")
        } catch {
            NSLog("[WZ] local server failed to start: \(error)")
        }
    }

    private func accept(_ conn: NWConnection) {
        conn.start(queue: queue)
        readRequest(conn, buffer: Data())
    }

    private func readRequest(_ conn: NWConnection, buffer: Data) {
        conn.receive(minimumIncompleteLength: 1, maximumLength: 64 * 1024) { [weak self] data, _, isComplete, error in
            guard let self = self else { conn.cancel(); return }
            var buf = buffer
            if let data = data { buf.append(data) }
            if let sep = buf.range(of: Data("\r\n\r\n".utf8)) {
                let head = String(data: buf.subdata(in: buf.startIndex..<sep.lowerBound), encoding: .utf8) ?? ""
                let firstLine = head.components(separatedBy: "\r\n").first ?? ""
                let parts = firstLine.components(separatedBy: " ")
                let path = parts.count >= 2 ? parts[1] : "/"
                self.serve(conn, requestPath: path)
            } else if isComplete || error != nil {
                conn.cancel()
            } else {
                self.readRequest(conn, buffer: buf)
            }
        }
    }

    private func serve(_ conn: NWConnection, requestPath: String) {
        var path = requestPath
        if let q = path.firstIndex(of: "?") { path = String(path[..<q]) }
        path = path.removingPercentEncoding ?? path
        if path == "/" || path.isEmpty { path = "/index.html" }
        // Basic traversal guard
        let clean = path.replacingOccurrences(of: "..", with: "")
        let fileURL = root.appendingPathComponent(clean)

        guard let body = try? Data(contentsOf: fileURL) else {
            let nf = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"
            conn.send(content: Data(nf.utf8), completion: .contentProcessed { _ in conn.cancel() })
            return
        }

        var header = "HTTP/1.1 200 OK\r\n"
        header += "Content-Type: \(Self.mime(fileURL.pathExtension))\r\n"
        header += "Content-Length: \(body.count)\r\n"
        // Cross-origin isolation (SharedArrayBuffer / WASM threads)
        header += "Cross-Origin-Opener-Policy: same-origin\r\n"
        header += "Cross-Origin-Embedder-Policy: require-corp\r\n"
        header += "Cross-Origin-Resource-Policy: same-origin\r\n"
        header += "Cache-Control: no-cache\r\n"
        header += "Connection: close\r\n\r\n"
        var out = Data(header.utf8)
        out.append(body)
        conn.send(content: out, completion: .contentProcessed { _ in conn.cancel() })
    }

    static func mime(_ ext: String) -> String {
        switch ext.lowercased() {
        case "html", "htm": return "text/html; charset=utf-8"
        case "js", "mjs":   return "text/javascript; charset=utf-8"
        case "wasm":        return "application/wasm"
        case "json":        return "application/json"
        case "data", "bin", "mem": return "application/octet-stream"
        case "png":  return "image/png"
        case "jpg", "jpeg": return "image/jpeg"
        case "svg":  return "image/svg+xml"
        case "gif":  return "image/gif"
        case "css":  return "text/css; charset=utf-8"
        case "ico":  return "image/x-icon"
        case "wav":  return "audio/wav"
        case "ogg", "oga": return "audio/ogg"
        case "mp3":  return "audio/mpeg"
        case "ttf":  return "font/ttf"
        case "woff": return "font/woff"
        case "woff2": return "font/woff2"
        default:     return "application/octet-stream"
        }
    }
}

#if compiler(>=6.1)
@objc @implementation
#else
@_objcImplementation
#endif
extension MainViewController {
}

class ViewController: MainViewController {
    override func viewDidLoad() {
        super.viewDidLoad()

        // Start the embedded loopback server (once) serving the bundled www.
        if gWZServer == nil, let www = Bundle.main.resourceURL?.appendingPathComponent("www") {
            let s = WZLocalServer(root: www, port: kWZPort)
            s.start()
            gWZServer = s
        }

        // Point the WebView at the http://localhost origin instead of app://
        // so the page is cross-origin isolated and WASM threads work.
        DispatchQueue.main.async { [weak self] in
            guard let self = self, let url = URL(string: "http://localhost:\(kWZPort)/index.html") else { return }
            _ = self.webViewEngine.load(URLRequest(url: url))
        }
    }
}
