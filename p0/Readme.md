# P0 - Basic UDP Authentication Token Generator

An authenticator of student groups. The authentication protocol is capable 
of authenticating students individually or in groups.

## Description

The authentication protocol used in this assignment is a request-response protocol. 
First, the client requests the server an authentication token. Second, after receiving 
the authentication token, the client can authenticate itself infinite times using the token,
for example, to access other functionalities in the application.

The communication protocol uses UDP messages. 
Error detection and message retransmission in case of failure 
is the responsibility of the client.

## Command-Line Interface

The client program should receive three positional arguments:

./client <host> <port> <command>

Where host and port are those for the authentication server above.
The command parameter specifies which command will be executed. In particular, 
the program must support the four commands below:

- itr <id> <nonce>

This command should send an individual token request to the server. 
The SAS (Student Authentication Sequence) received from the server 
must be printed as the program's output.

- itv <SAS>

This command should send an individual token validation message to the server for 
the SAS given on the command line. The validation result must be printed as 
the program's output.

- gtr <N> <SAS-1> <SAS-2> ... <SAS-N>

This command should send a group token request to the server. 
The parameter N gives the number of SAS that will be sent to the server. 
The GAS (Group Authentication Sequence) received from the server should be printed as the program's output.

gtv <GAS>

This command should send a group token validation message to the server. 
The validation result should be printed as the program's output.

On the command line and program output, SAS and GAS should be informed in a 
standardized manner, as follows. SAS fields should be separated by a colon (:), 
and be printed as a string:

<id>:<nonce>:<token>

Similarly, GAS should be printed with the SAS and token separated 
by a plus sign (+). Each SAS should be printed as specified above. For example:

<SAS1>+<SAS2>+<SAS3>+<token>

Note that there are no spaces in SAS and GAS.

## Usage Example

Individual Token Request
```sh
./client vcm-23691.vm.duke.edu 51001 itr ifs4 1
```
Reponse:
ifs4:1:2c3bb3f0e946a1afde7d9d0c8c818762a6189e842abd8aaaf85c9faac5b784d2


Individual Token Validation
```sh
% ./client vcm-23691.vm.duke.edu 51001 itv ifs4:1:2c3bb3f0e946a1afde7d9d0c8c818762a6189e842abd8aaaf85c9faac5b784d2
```
Response:
0

Group Token Request
```sh
./client vcm-23691.vm.duke.edu 51001 gtr 2 ifs4:1:2c3bb3f0e946a1afde7d9d0c8c818762a6189e842abd8aaaf85c9faac5b784d2 ifs4:2:cf87a60a90159078acecca4415c0331939ebb28ac5528322ac03d7c26b140b98
```
Response:
ifs4:1:2c3bb3f0e946a1afde7d9d0c8c818762a6189e842abd8aaaf85c9faac5b784d2+ifs4:2:cf87a60a90159078acecca4415c0331939ebb28ac5528322ac03d7c26b140b98+e51d06a4174b5385c8daff714827b4b4cb4f93ff1b83af86defee3878c2ae90f


Group Token Validation
```sh
./client vcm-23691.vm.duke.edu 51001 gtv ifs4:1:2c3bb3f0e946a1afde7d9d0c8c818762a6189e842abd8aaaf85c9faac5b784d2+ifs4:2:cf87a60a90159078acecca4415c0331939ebb28ac5528322ac03d7c26b140b98+e51d06a4174b5385c8daff714827b4b4cb4f93ff1b83af86defee3878c2ae90f
```
Response:
0

