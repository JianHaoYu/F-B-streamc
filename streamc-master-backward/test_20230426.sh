#!/bin/bash
# 当前路径
d_dir=$(pwd)
# 大文件夹名字
g_dir="bg_dir_b"

if [ -d ${d_dir}"/"${g_dir} ]; then
	echo "大目录已存在"
else
	mkdir ${d_dir}"/"${g_dir} 
	echo "大目录已创建"
fi


for j in $(seq 0.01 0.01 0.21)
do
    for i in $(seq 1 100)
    do
		# 先判断文件夹是否存在
		if [ -d ${d_dir}"/"${g_dir}"/data_"${j} ]; then
			echo "小目录已存在"
		else
			mkdir ${d_dir}"/"${g_dir}"/data_"${j}
		echo "小目录已创建"
		fi
	j_plus_02=$(echo "$j + 0.01" | bc)		
        ./streamcTest 1000 0 0.22 $j 1000 0 | tee ${d_dir}"/"${g_dir}"/data_"${j}"/"data_${i}_${j}.txt

    done
done
