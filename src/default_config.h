#pragma once
#include <string>

const std::string default_config = R"(
[webserverserver]
port=8080
compression=1
max_payload_length=2048
reset_idle_timeout_on_send=true
idle_timeout=120
max_backpressure=65536
log_folder=logs
max_messages_per_second=50000
message_queue_length=250000

[ssl]
enabled=false
key_filename=certs/key.pem
cert_filename=certs/cert.pem
passphrase=123456

[games]
scripts_folder=scripts
log_folder=logs
listing_interval=1000
max_reconnection_time=360000

[database]
enabled=false
username=<username>
password=<password>
host=<host>
port=<port>
database=<database>
sslmode=<sslmode>

[authentication]
enabled=false

[analytics]
enabled=false
access_token=<access_token>
secret_key=<secret_key>

[license]
id=f2867f41-29c8-4bfb-93b9-436f71f921dc
)";
