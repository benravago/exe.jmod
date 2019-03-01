# Coding exercise notes

## 1. Source files

1. __rc/ams/app/Main.java__
<br>This is the primary application service class.
    It contains the process main() entry point which configures the spark framework environment to:
    a) use the port 8080;
    b) recognize the route "api/tou/changes";
    c) capture exceptions and pass back a relevant http 500 message.
    
2. __src/ams/app/handlers/TOU.java__
<br>This is the class which holds the 'api/tou/*' route handlers. The only one implemented is for the 'changes' function.
    This handler computes the Time-of-Use period response from supplied time span query paramaters and the input season data.

## 2. Dependencies

1.  The code was written for JDK10 (as that's what I have on my workstation).
    However, the only actual JDK10 dependency is the use of the new 'var' keyword.
    Outside of that, the code should work with JDK8.

2.  The application uses the spark framework for its REST/microservice implementation.
    This in turn requires (at a minimum) slf4j, jetty and the servlet api.
    
3.  For JSON functions, the application uses the json-p reference implementation from the glassfish project.

4. These dependencies can all be downloaded from public maven repositories, for example:
   <br> 
   <br> http://central.maven.org/maven2/com/sparkjava/spark-core/2.8.0/spark-core-2.8.0.jar
   <br> http://central.maven.org/maven2/org/slf4j/slf4j-api/1.7.25/slf4j-api-1.7.25.jar
   <br> http://central.maven.org/maven2/org/slf4j/slf4j-simple/1.7.25/slf4j-simple-1.7.25.jar
   <br> http://central.maven.org/maven2/org/eclipse/jetty/jetty-http/9.4.12.v20180830/jetty-http-9.4.12.v20180830.jar
   <br> http://central.maven.org/maven2/org/eclipse/jetty/jetty-server/9.4.12.v20180830/jetty-server-9.4.12.v20180830.jar
   <br> http://central.maven.org/maven2/org/eclipse/jetty/jetty-util/9.4.12.v20180830/jetty-util-9.4.12.v20180830.jar
   <br> http://central.maven.org/maven2/org/eclipse/jetty/jetty-io/9.4.12.v20180830/jetty-io-9.4.12.v20180830.jar
   <br> http://central.maven.org/maven2/javax/servlet/javax.servlet-api/3.1.0/javax.servlet-api-3.1.0.jar
   <br> http://central.maven.org/maven2/org/glassfish/javax.json/1.0.4/javax.json-1.0.4.jar
   <br> http://central.maven.org/maven2/javax/json/javax.json-api/1.0/javax.json-api-1.0.jar

## 3. Setup

1. An example ./setup script is include.
   This script will create and populate a ./lib classpath directory with the required jar files mentioned above,
   then compile and run the Main.java program.
   
2. The ./setup script will also run some tests (via 'curl') using the data in the example/README.cmd file.
   
3. The ./setup script can also be used as a guide for building and running the code via an IDE.

4. To run the ./setup script, first edit the file and set the appropriate path for the JDK= variable,
   then just run './setup' from the command line.
   
   
   


