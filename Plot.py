import matplotlib.pyplot as plt
from numpy import *
import math

fig, axs = plt.subplots(2, 2,figsize=(14,12))

RepairFFIX = [0.05]
multiply_add_avergaeFFIX=[]
Linun_multiply_addFFIX=[]

def handletestFFIX(repair,i):
    RepairFFIX.append(1/(repair+1) )  
    SourceLossSum=0.0
    RepairID_x=0.0
    OVER=False

    multiply_add =[]

    filename = "/home/mininet/1111/TEST_Paper2/FixData2/F_Fix"+"_"+str(repair)+"_0.01.log"
    with open(filename,'r') as file_object:
        for line in file_object:
            linecopy = line
            information = linecopy.split( )
            if information[0]=="[Channel]":
                SourceLossSum+=1
            if information[0]=="[Decoder]" and information[1]=="Processed":
                RepairID_x = float(information[4])
            if information[0]=="[ROWOPTION]" and information[1]=="Inactivating": 
                multiply_add_Inactive =float(information[29])
                n_row_multiply_add_A = float(information[27])
                multiply_add.append(n_row_multiply_add_A+multiply_add_Inactive)
            if information[0] =="[Summary]":
                OVER=True
    file_object.close()

    if not OVER :
        print(str(repair)+"Not Finish!")

    MA_I_mean =mean(multiply_add)

    TrueLoss = SourceLossSum/1000000
    TrueRepair = (RepairID_x+1)/(RepairID_x+1+1000000)

    multiply_add_avergaeFFIX.append(MA_I_mean)
    
    PS=200
    D=repair
    mu =1/D
    Lambda = (1-TrueRepair)*TrueLoss
    p =(Lambda/mu)

    n_Liulun=1/(1-p)
    n_D_Liulun = p/math.pow(1-p,3)+n_Liulun*n_Liulun

    Inactive_Liln =(1/mu+p/2/(mu*(1-p)))*n_Liulun*PS
    subLiluninterval = (p)/2/(mu*(1-p))/(n_Liulun-1)
    Lilun =subLiluninterval*(n_D_Liulun-n_Liulun)/2
    A_Liulun= Lilun*201+(p/2/(mu*(1-p)))*(n_D_Liulun-n_Liulun)/2

    Linun_multiply_addFFIX.append(A_Liulun+Inactive_Liln)


def testFFIX():
    test2repair = 19
    while test2repair < 74:
        test2repair = test2repair+1
        handletestFFIX(test2repair,0)
testFFIX()

multiply_add_avergaeFFIX.insert(0,multiply_add_avergaeFFIX[0])
Linun_multiply_addFFIX.insert(0,Linun_multiply_addFFIX[0])

axs[0, 0].plot(RepairFFIX,multiply_add_avergaeFFIX,label="Simulation")
axs[0, 0].plot(RepairFFIX,Linun_multiply_addFFIX,label="Analysis")
axs[0, 0].set_xlabel("repair packet insertion percentage, f\n(a)", fontsize=20)
axs[0, 0].set_ylabel("$N_{ma}^{F}$ Fixed-Interval", fontsize=20)
axs[0, 0].set_xlim(0.013334,0.05)
axs[0, 0].grid(True)
axs[0, 0].legend(fontsize=20)
axs[0, 0].tick_params(labelsize=15)



RepairBFIX =[0.05]
Lilun_interval_avergaeBFIX=[]
multiply_add_avergaeBFIX=[]

def handletestBFIX(repair):
    RepairBFIX.append(1/(repair+1))  
    LostSum=0
    LastPacktID =0

    multiply_add =[]
    filename = "/home/mininet/1111/TEST_Paper2/FixData2/B_Fix"+"_"+str(repair)+"_0.01.log"
    with open(filename,'r') as file_object:
        for line in file_object:
            linecopy = line
            information = linecopy.split( )
            if information[0]=="[Channel]":
                LostSum+=1
            if information[0]=="[Decoder]":
                LastPacktID =int(information[4])

            if information[0]=="[ROWOPTION]" and information[1]=="Inactivating": 
                multiply_add_Inactive =float(information[29])
                n_row_multiply_add_A = float(information[27])
                multiply_add.append(multiply_add_Inactive+n_row_multiply_add_A)

    file_object.close()

    MA_I_mean =mean(multiply_add)
    multiply_add_avergaeBFIX.append(MA_I_mean)

    TrueLoss =LostSum/1000000
    TrueRepair =(LastPacktID+1)/(LastPacktID+1+1000000)
    PS=200
    D=repair
    mu = 1/D
    Lambda = (1-TrueRepair)*TrueLoss
    p =(Lambda/mu)

    n_Liulun=1/(1-p)
    n_D_Liulun = p/math.pow(1-p,3)+n_Liulun*n_Liulun

    subLiluninterval = p/2/(mu*(1-p))/(n_Liulun-1)
    Lilun =subLiluninterval*(n_D_Liulun-n_Liulun)/2
    Inactive_Liln =Lilun*PS
    Liulun =(1/mu+p/2/(mu*(1-p)))*n_Liulun

    A_Liulun= Liulun*(PS+1)

    Lilun_interval_avergaeBFIX.append(A_Liulun+Inactive_Liln)

def testBFIX():
    test2repair = 19
    while test2repair < 74:
        test2repair = test2repair+1
        handletestBFIX(test2repair)
testBFIX()

multiply_add_avergaeBFIX.insert(0,multiply_add_avergaeBFIX[0])
Lilun_interval_avergaeBFIX.insert(0,Lilun_interval_avergaeBFIX[0])

axs[0, 1].plot(RepairBFIX,multiply_add_avergaeBFIX,label="Simulation")
axs[0, 1].plot(RepairBFIX,Lilun_interval_avergaeBFIX,label="Analysis")
axs[0, 1].set_xlabel("repair packet insertion percentage, f\n(b)", fontsize=20)
axs[0, 1].set_ylabel("$N_{ma}^{B}$ Fixed-Interval", fontsize=20)
axs[0, 1].set_xlim(0.01334,0.05)
axs[0, 1].grid(True)
axs[0, 1].legend(fontsize=20)
axs[0, 1].tick_params(labelsize=15)



Repair = []
multiply_add_avergae=[]
Linun_multiply_add=[]

def GetLilunEA(f,e):
    lamda=e*(1-f)
    mu = (1-e)*f
    p =(lamda/mu)

    EN=1/(1-p)
    E2N=2*p/math.pow(1-p,3)+(EN)
    return (E2N-EN)/2

def GetLilunEAP2(f,e):
    Lambda = (1-f)*e
    mu = (1-e)*f
    LiLuninterval = (Lambda/mu)*(1/(mu-Lambda))
    n_Lilun = (1/(f-e)-1)*e+1
    subLiluninterval = LiLuninterval/(n_Lilun-1)
    Lilun =subLiluninterval*GetLilunEA(f,e)
    return Lilun*201

def GetLilunEAP1(f,e):
    mu =(1-e)*f
    EA = (1/mu)*((1/(f-e)-1)*e+1)
    EA = GetLilunEA(f,e)*EA
    return EA


def handletestF(repair):
    Repair.append(repair)    
    SourceLossSum=0.0
    RepairID_x=0.0

    multiply_add =[]
    filename = "/home/mininet/1111/streamc-master/F_Random_"+str(repair)+"_0.01.log"
    with open(filename,'r') as file_object:
        for line in file_object:
            linecopy = line
            information = linecopy.split( )
            if information[0]=="[Channel]":
                SourceLossSum+=1
            if information[0]=="[Decoder]" and information[1]=="Processed":
                RepairID_x = float(information[4])

            if information[0]=="[ROWOPTION]" and information[1]=="Inactivating": 
                multiply_add_Inactive =float(information[27])
                n_row_multiply_add_A = float(information[29])
                multiply_add.append(multiply_add_Inactive+n_row_multiply_add_A)
    file_object.close()

    MA_I_mean =mean(multiply_add)
    multiply_add_avergae.append(MA_I_mean)

    TrueLoss = SourceLossSum/1000000
    TrueRepair = (RepairID_x+1)/(RepairID_x+1+1000000)
    
    PS=200
    DW=1/(TrueRepair-TrueLoss)
    n_Liulun = (DW-1)*TrueLoss +1
    mu = (1-TrueLoss)*TrueRepair
    Lambda = (1-TrueRepair)*TrueLoss

    Inactive_Liln =(1/mu)*n_Liulun*n_Liulun*PS
    LiulunP1 = GetLilunEAP1(TrueRepair,TrueLoss)
    LiulunP2 = GetLilunEAP2(TrueRepair,TrueLoss)
    A_Liulun=LiulunP1+LiulunP2

    Linun_multiply_add.append(Inactive_Liln+A_Liulun)



def test3():
    test2repair = 12
    while test2repair < 100:
        test2repair = test2repair+1
        handletestrepair = test2repair/1000
        handletestF(handletestrepair)
test3()


axs[1, 0].plot(Repair,multiply_add_avergae,label="Simulation")
axs[1, 0].plot(Repair,Linun_multiply_add,label="Analysis")
axs[1, 0].set_xlabel("repair packet insertion percentage, f\n(c)", fontsize=20)
axs[1, 0].set_ylabel("$N_{ma}^{F}$ Random", fontsize=20)
axs[1, 0].set_xlim(0.013,0.05)
axs[1, 0].grid(True)
axs[1, 0].legend(fontsize=20)
axs[1, 0].tick_params(labelsize=15)



def GetBLilunEA(f,e):
    lamda=e*(1-f)
    mu = (1-e)*f
    p =(lamda/mu)

    EN=1/(1-p)
    E2N=2*p/math.pow(1-p,3)+(EN)
    return (E2N-EN)/2

def GetBLilunEAP1(f,e):
    mu = (1-e)*f
    Lambda = (1-f)*e

    LiLuninterval = (Lambda/mu)*(1/(mu-Lambda))
    n_Lilun = (1/(f-e)-1)*e+1
    subLiluninterval = LiLuninterval/(n_Lilun-1)
    Lilun = subLiluninterval*GetLilunEA(f,e)
    Lilun += (1/mu)*n_Lilun*n_Lilun*201
    return Lilun

BRepair =[]
BLilun_interval_avergae=[]
Bmultiply_add_avergae=[]

def handletest(repair):
    BRepair.append(repair)
    LostSum=0
    LastPacktID =0

    multiply_add =[]
    filename = "/home/mininet/1111/streamc-master-backward/B_Random_"+str(repair)+"_0.01.log"
    with open(filename,'r') as file_object:
        for line in file_object:
            linecopy = line
            information = linecopy.split( )
            if information[0]=="[Channel]":
                LostSum+=1
            if information[0]=="[Decoder]":
                LastPacktID =int(information[4])

            if information[0]=="[ROWOPTION]" and information[1]=="Inactivating": 
                multiply_add_Inactive =float(information[29])
                n_row_multiply_add_A = float(information[27])
                multiply_add.append(multiply_add_Inactive+n_row_multiply_add_A)

    file_object.close()

    MA_I_mean =mean(multiply_add)
    Bmultiply_add_avergae.append(MA_I_mean)


    PS=200
    TrueLoss =LostSum/1000000
    TrueRepair =(LastPacktID+1)/(LastPacktID+1+1000000)
    Lambda = (1-TrueRepair)*TrueLoss
    mu = (1-TrueLoss)*TrueRepair
    LiLuninterval = (Lambda/mu)*(1/(mu-Lambda))
    n_Lilun = (1/(TrueRepair-TrueLoss)-1)*TrueLoss+1
    subLiluninterval = LiLuninterval/(n_Lilun-1)
    Inactive_Liln =subLiluninterval*GetBLilunEA(TrueRepair,TrueLoss)*(PS)
    A_Liulun = GetBLilunEAP1(TrueRepair,TrueLoss)
    BLilun_interval_avergae.append(A_Liulun+Inactive_Liln)


def test4():
    test2repair = 12
    while test2repair < 100:
        test2repair = test2repair+1
        handletestrepair = test2repair/1000
        handletest(handletestrepair)
test4()


axs[1, 1].plot(BRepair,Bmultiply_add_avergae,label="Simulation")
axs[1, 1].plot(BRepair,BLilun_interval_avergae,label="Analysis")
axs[1, 1].set_xlabel("repair packet insertion percentage, f\n(d)", fontsize=20)
axs[1, 1].set_ylabel("$N_{ma}^{B}$ Random", fontsize=20)
axs[1, 1].set_xlim(0.013,0.05)
axs[1, 1].grid(True)
axs[1, 1].legend(fontsize=20)
axs[1, 1].tick_params(labelsize=15)
plt.tight_layout()
plt.show()
