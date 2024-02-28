# F-B-streamc on ubuntu

To build step by step, you should:
```shell
$ git clone https://github.com/JianHaoYu/F-B-streamc.git
$ cd streamc-master
$ make streamcTest
$ cd ..
$ cd streamc-master-backward
$ make streamcTest
$ cd ..
```
Afterwards, run test.py to run the test, please pay attention to the path of streamcTest.
```shell
$ python3 Test.py
```
Finally, run plot.py to display the results, please pay attention to the path of the test file.
```shell
$ python3 Plot.py
```