# ydfs
ydfs is an open source mini distributed file system. It's major functions include: file storing, file syncing, and design for high capacity and load balance.

ydfs is designed for c10k problem,When connected 10000 clients,it runs well.

I test with Centos 6.3 on my laptop:

Thinkpad X230

Intel(R) Core(TM) i5-3210M CPU @ 2.50GHz 2 cores

4G RAM

when run 1 tracker,2 storage,10000 client on one computer,every client upload 10 files in the same time

this is the result:

group_name-0

storage_id-0--------ip-127.0.0.1:9000--------size-11426400000--------upload_size-5282424720

count_stamp:NULL--------00000C1d--------NULL---------------status:ONLINE

storage_id-1--------ip-127.0.0.1:9001--------size-11426400000--------upload_size-6143975280

count_stamp:00000DzF--------NULL--------NULL---------------status:ONLINE

group_size 22852800000--------group_upload_size 11426400000

upload_size---- 11426400000--------upload_sec---- 335--------upload_speed----- 32.528358M/s

download_size 11426400000--------download_sec 536--------download_speed- 20.330223M/s

NOTE:

(1)When uploading,sync will catch some speed.In my computer,when uploading finished,the group_size is 1.5 times of the group_upload_size.When you test without syncing,It will be faster than it.

(2)Download_speed is slower than upload_speed,because when uploading,os will cache the file you want upload,the computer just need to do something write to disk. But when downloading,the computer not only reading from disk,but also writing to disk,so 

it is slower than upload.

wait for more test results
