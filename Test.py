import subprocess
import numpy as np

n=10

def handletest_FixInterval(repair,loss):
    for i in range(0,n):
        command = "cd /home/mininet/1111/streamc-master/;./streamcTest 1000000 0 "+str(repair)+" "+str(loss)+" 100 "+str(i+3)+" 0 | egrep '\[Channel\] Source |\[Decoder\] Processed repair |\[ROWOPTION\] Inactivating |\[Summary\] Free decoder...'> /home/mininet/1111/TEST_Paper2/FixData2/F_Fix"+"_"+str(repair)+"_"+str(loss)+".log"
        result = subprocess.run(command, shell=True, capture_output=True, text=True)
        if result.returncode!=0:
            print("forward "+str(i))
            print("标准输出:", result.stdout)
            print("错误输出:", result.stderr)
            print("返回码:", result.returncode)

        command = "cd /home/mininet/1111/streamc-master-backward/;./streamcTest 1000000 0 "+str(repair)+" "+str(loss)+" 100 "+str(i+3)+" 0 | egrep '\[Channel\] Source |\[Decoder\] Processed repair |\[ROWOPTION\] Inactivating |\[Summary\] Free decoder...'> /home/mininet/1111/TEST_Paper2/FixData2/B_Fix"+"_"+str(repair)+"_"+str(loss)+".log"
        result = subprocess.run(command, shell=True, capture_output=True, text=True)
        if result.returncode!=0:
            print("forward "+str(i))
            print("标准输出:", result.stdout)
            print("错误输出:", result.stderr)
            print("返回码:", result.returncode)


def handletest_Random(repair,loss):
    for i in range(0,n):
        command = "cd /home/mininet/1111/streamc-master/;./streamcTest 1000000 0 "+str(repair)+" "+str(loss)+" 100 "+str(i+3)+" 0 | egrep '\[Channel\] Source |\[Decoder\] Processed repair |\[ROWOPTION\] Inactivating |\[Summary\] Free decoder...'> /home/mininet/1111/TEST_Paper2/FixData2/F_Random"+"_"+str(repair)+"_"+str(loss)+".log"
        result = subprocess.run(command, shell=True, capture_output=True, text=True)
        if result.returncode!=0:
            print("forward "+str(i))
            print("标准输出:", result.stdout)
            print("错误输出:", result.stderr)
            print("返回码:", result.returncode)

        command = "cd /home/mininet/1111/streamc-master-backward/;./streamcTest 1000000 0 "+str(repair)+" "+str(loss)+" 100 "+str(i+3)+" 0 | egrep '\[Channel\] Source |\[Decoder\] Processed repair |\[ROWOPTION\] Inactivating |\[Summary\] Free decoder...'> /home/mininet/1111/TEST_Paper2/FixData2/B_Random"+"_"+str(repair)+"_"+str(loss)+".log"
        result = subprocess.run(command, shell=True, capture_output=True, text=True)
        if result.returncode!=0:
            print("forward "+str(i))
            print("标准输出:", result.stdout)
            print("错误输出:", result.stderr)
            print("返回码:", result.returncode)


def test1():
    test2repair = 19
    while test2repair < 74:
        test2repair = test2repair+1
        handletest_FixInterval(test2repair,0.01)

def test2():
    test2repair = 14
    while test2repair < 300:
        test2repair = test2repair+1
        handletestrepair = test2repair/1000
        handletest_Random(handletestrepair,0.01)

test1()
test2()








