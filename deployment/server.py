from flask import Flask, request
import socket

app = Flask(__name__)

# 获取本机 IP 地址，方便你查看填入 ESP32
def get_host_ip():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(('8.8.8.8', 80))
        ip = s.getsockname()[0]
    finally:
        s.close()
    return ip

@app.route('/upload', methods=['POST'])
def upload_file():
    # 接收原始二进制数据
    data = request.get_data()
    data_len = len(data)
    
    print(f"✅ [服务端] 收到数据包! 大小: {data_len} bytes")
    
    # 你甚至可以把图片保存下来验证完整性
    # with open(f"received_pest.jpg", "wb") as f:
    #     f.write(data)
        
    return "Upload Success", 200

if __name__ == '__main__':
    host_ip = get_host_ip()
    print(f"\n========================================")
    print(f"🚀 服务器已启动!")
    print(f"🏠 本机 IP: {host_ip}")
    print(f"🔗 ESP32 请配置 URL: http://{host_ip}:8000/upload")
    print(f"========================================\n")
    # host='0.0.0.0' 允许外部设备访问
    app.run(host='0.0.0.0', port=8000, debug=False)