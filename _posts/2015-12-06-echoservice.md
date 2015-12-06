---
layout: page
title: "Echo Service"
category: tut
date: 2015-12-06 13:31:10
---


### Basics of a plugin

Developing an http service starts with deriving a class from [pion::http::plugin_service]({{site.baseurl}}/api/user/html/classpion_1_1http_1_1plugin__service.html) and implementing the () operator. Then spending time with
the two arguments: const [pion::http::request_ptr]({{site.baseurl}}/api/user/html/classpion_1_1http_1_1request.html)& http_request_ptr and const [pion::tcp::connection_ptr]({{site.baseurl}}/api/user/html/classpion_1_1tcp_1_1connection.html)& tcp_conn.  You have all you need to stand up a service.
To remain uncomplicated, use the same pion::plugins namepaces.

```
namespace pion {        // begin namespace pion
namespace plugins {     // begin namespace plugins
```
In the initialization area of the class definition start with the public pion::http::plugin_service in your [EchoService.hpp](https://github.com/splunk/pion/blob/develop/services/EchoService.hpp) file

```
class EchoService :
    <b>public pion::http::plugin_service</b>
```
and overide the virtual () operator in your [EchoService.cpp](https://github.com/splunk/pion/blob/develop/services/EchoService.cpp) file within the same namespaces.

```
void EchoService::operator()(const http::request_ptr& http_request_ptr, const tcp::connection_ptr& tcp_conn)
{
            std::cout<<"Method: "<<http_request_ptr->get_method()<<std::endl;
            std::cout<<"Resource: "<<http_request_ptr->get_resource()<<std::endl;
}
```
