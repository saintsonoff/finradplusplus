from http.server import HTTPServer, BaseHTTPRequestHandler
import json
from datetime import datetime

class WebhookHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        if self.path == '/webhook/alerts':
            content_length = int(self.headers['Content-Length'])
            body = self.rfile.read(content_length)
            
            try:
                data = json.loads(body)
                timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                
                print(f"\n{'='*80}")
                print(f"WEBHOOK ALERT RECEIVED at {timestamp}")
                print(f"{'='*80}")
                
                if 'alerts' in data:
                    for alert in data['alerts']:
                        status = alert.get('status', 'unknown')
                        labels = alert.get('labels', {})
                        annotations = alert.get('annotations', {})
                        
                        print(f"\n Alert: {labels.get('alertname', 'Unknown')}")
                        print(f"   Status: {status}")
                        print(f"   Severity: {labels.get('severity', 'unknown')}")
                        print(f"   Summary: {annotations.get('summary', 'N/A')}")
                        print(f"   Description: {annotations.get('description', 'N/A')}")
                        
                        if 'startsAt' in alert:
                            print(f"   Started: {alert['startsAt']}")
                
                print(f"\n{'='*80}")
                print(f"Full JSON:")
                print(json.dumps(data, indent=2))
                print(f"{'='*80}\n")
                
                self.send_response(200)
                self.send_header('Content-type', 'application/json')
                self.end_headers()
                self.wfile.write(json.dumps({"status": "ok"}).encode())
                
            except Exception as e:
                print(f"Error processing webhook: {e}")
                self.send_response(500)
                self.end_headers()
        else:
            self.send_response(404)
            self.end_headers()
    
    def log_message(self, format, *args):
        pass

if __name__ == '__main__':
    port = 8888
    server = HTTPServer(('0.0.0.0', port), WebhookHandler)
    print(f"Webhook server started on port {port}")
    print(f"Listening for alerts at http://localhost:{port}/webhook/alerts")
    print(f"{'='*80}\n")
    
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n\nServer stopped")
        server.shutdown()
