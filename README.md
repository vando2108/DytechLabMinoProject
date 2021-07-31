# HTTP server
1. You can config the number of threads and number of tasks in threadpool.h file.
2. To run the server, you need to run execute file in the "Bin" folder or you can rebuild by run "make" command.
3. This server has 3 URL you can access:
- "127.0.0.1:8080" to get index.html file.
- "127.0.0.1:8080/login" to get login page.
- "127.0.0.1:8080/register" to get register page.
- The other URL will get 404-page not found error
4. The login and register services just are the basic services, I don't use any database or hashing technical in these services.
5. 
