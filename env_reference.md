# Environment Variables Reference
## Notes
* Any boolean environment variable will count as true the next values (not case-sensitive): 
  * true
  * 1
  * yes
  * y
  * on
* The next .env files will be read:
  1. Root of the project (or from wherever the application is executed)
  2. If it contains the absolute path to the component's directory, the env file in that directory will be read as well. If there's repeated variables, the one in the component's directory will take precedence.

## Common
* DEBUG: 
  * Description: whether to print debug messages 
  * Value: boolean
  * Default value: false
* KAFKA_LOG_PATH: 
  * Description: path to the Kafka logs 
  * Value: path/to/existing/directory
  * Default value: kafka_logs
  * Considerations:
    * If the path doesn't exist, logs will simply not be written.
* REDIRECT_KAFKA_LOGS:
  * Description: whether to redirect Kafka logs to the file indicated by KAFKA_LOG_PATH
  * Value: boolean
  * Default value: false
* KAFKA_BOOTSTRAP_SERVER:
  * Description: Kafka server address
  * Value: host:port
  * Default value: 127.0.0.1:9092
  * Considerations: 
    * Please use 127.0.0.1 instead of localhost to avoid issues where localhost might be resolved as an IPv6 address.
* MYSQL_SERVER:
  * Description: MySQL server address
  * Value: host:port
  * Default value: 127.0.0.1:3306