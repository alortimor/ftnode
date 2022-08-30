This project is essentially a test for the viability of the DivSQL
protocol. It was previously developed in Java, which used RMI as the central
middleware. This project uses C++ along with SQLAPI++ library instead.

A DivRep controller, which consists of an adjudicator, a database executor and an sql formatter, actions an incoming client database database request. The controller actions the request on multiple off the shelf databases, for example PostSQL, Oracle, SQL Anywhere etc. All daabases support snapshot isolation. The goal being to compare and contrast the data of each databases by using hash techniques, after each execution. An execution may involve a vector of SQL statements or a database BEGIN END block. This ultimate aim is to achieve a level of fault tolerance across more than a single disparate Relation database.

The client to server communication is managed by a TCP/IP server developed using the Boost library.
