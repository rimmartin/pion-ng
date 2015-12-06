---
layout: page
title: "Learning from Echo Service"
category: tut
date: 2015-12-06 13:31:10
---


### Basics of a plugin

Developing an http service starts with deriving a class from [pion::http::plugin_service]({{site.baseurl}}/api/user/html/classpion_1_1http_1_1plugin__service.html) and implementing the () operator. Then spending time with
the two arguments: const [pion::http::request_ptr]({{site.baseurl}}/api/user/html/classpion_1_1http_1_1request.html)& http_request_ptr and const [pion::tcp::connection_ptr]({{site.baseurl}}/api/user/html/classpion_1_1tcp_1_1connection.html)& tcp_conn [#1](#note1).  You have all you need to stand up a service.
To remain uncomplicated, use the same pion::plugins namepaces.

```
namespace pion {        // begin namespace pion
namespace plugins {     // begin namespace plugins
```
In the initialization area of the class definition start with the public pion::http::plugin_service in your [EchoService.hpp](https://github.com/splunk/pion/blob/develop/services/EchoService.hpp) file

```cpp
class EchoService :
    **public pion::http::plugin_service**
```
and overide the virtual () operator in your [EchoService.cpp](https://github.com/splunk/pion/blob/develop/services/EchoService.cpp) file within the same namespaces.

```cpp
void EchoService::operator()(const http::request_ptr& http_request_ptr, const tcp::connection_ptr& tcp_conn)
{
            std::cout<<"Method: "<<http_request_ptr->get_method()<<std::endl;
            std::cout<<"Resource: "<<http_request_ptr->get_resource()<<std::endl;
}
```

This example assumes the REST method is a GET. You can return anything with [http::response_writer_ptr]({{site.baseurl}}/api/user/html/classpion_1_1http_1_1writer.html) you want or build the echo service that comes with pion and start it with piond.  The key is instantiating
a writer bound to the tcp connection, fill it in with some '<<' streaming and finally call writer->[send()]({{site.baseurl}}/api/user/html/classpion_1_1http_1_1writer.html#a6c96aa95d710babcf5096d8294f703d5)

```cpp
    // Set Content-type to "text/plain" (plain ascii text)
    http::response_writer_ptr writer(http::response_writer::create(tcp_conn, *http_request_ptr,
                                                            boost::bind(&tcp::connection::finish, tcp_conn)));
    writer->get_response().set_content_type(http::types::CONTENT_TYPE_TEXT);
    
```

An important section at the tail of [EchoService.cpp](https://github.com/splunk/pion/blob/develop/services/EchoService.cpp) after the namespaces close scope tells the pion daemon about the plugin and when you start making your own services
imitating it is where you either see it stand up or face quizzical nothing

```cpp
/// creates new EchoService objects
extern "C" PION_PLUGIN pion::plugins::EchoService *pion_create_EchoService(void)
{
    return new pion::plugins::EchoService();
}

/// destroys EchoService objects
extern "C" PION_PLUGIN void pion_destroy_EchoService(pion::plugins::EchoService *service_ptr)
{
    delete service_ptr;
}

```

Be sure you replace the word 'Echo' throughout the above if your service's suffix is 'Service', usually mirroring the resource you want to endpoint.  Get this correct and after that real meaningful errors are likely to occur as you work with your
request, connection and writer and you're in a better position to debug.

```bash
$PION_PATH/bin/piond --help
usage:   piond [OPTIONS] RESOURCE WEBSERVICE
         piond [OPTIONS] -c SERVICE_CONFIG_FILE
options: [-ssl PEM_FILE] [-i IP] [-p PORT] [-d PLUGINS_DIR] [-o OPTION=VALUE] [-v]
```

```bash
$PION_PATH/bin/piond -v -p 8080 -d $PION_PATH/share/pion/plugins /echo EchoService
```

and go to browser uri http://localhost:8080/echo to test and view the returned data/document. When you make your own on linux make sure the service shared library is missing the lib prefix by sym linking for example:

```bash
ln -sf ./libDescriptionLogicsService.so ./DescriptionLogicsService.so
```

or the daemon won't know it.

____
Note **<span id="note1">#1</span>**: the links to components point to the template X class reference documentation the typedefs like pion::http::request_ptr throughout pion code alias for boost::shared_ptr\<X\>'s.

