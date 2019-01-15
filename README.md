# xml_parser
xml文档解析，合法性检测，debug.xml为非法文档会给出详细提示 <br>
---
---
```
[yangyu@VM_0_3_centos xml_parser]$ 
[yangyu@VM_0_3_centos xml_parser]$ ./xml ./test3.xml 
========================== [ BEGIN  ] ==========================
########################## [ step 1 ] ##########################
			2019-01-15 13:20:27
			2019-01-15 13:20:27
########################## [ step 1 ] ##########################
########################## [ step 2 ] ##########################
check Info====> push:<dia:diagram>	 start:0
Info====> push:<dia:diagramdata>	 start:66
Info====> push:<dia:attribute1>	 start:89
Info====> pop :<dia:attribute1>	 start:89 end:210
Info====> pop :<dia:diagramdata>	 start:66 end:232
Info====> push:<dia:layer>	 start:237
Info====> push:<dia:object1>	 start:287
Info====> push:<dia:attribute2>	 start:351
Info====> pop :<dia:attribute2>	 start:351 end:489
Info====> push:<dia:attribute3>	 start:498
Info====> pop :<dia:attribute3>	 start:498 end:600
Info====> push:<dia:attribute4>	 start:609
Info====> pop :<dia:attribute4>	 start:609 end:796
Info====> push:<dia:attribute5>	 start:805
Info====> pop :<dia:attribute5>	 start:805 end:899
Info====> push:<dia:attribute6>	 start:908
Info====> pop :<dia:attribute6>	 start:908 end:997
Info====> push:<dia:attribute7>	 start:1006
Info====> pop :<dia:attribute7>	 start:1006 end:1093
Info====> push:<dia:attribute8>	 start:1102
Info====> pop :<dia:attribute8>	 start:1102 end:1190
Info====> push:<dia:attribute9>	 start:1199
Info====> pop :<dia:attribute9>	 start:1199 end:1285
Info====> push:<dia:connections1>	 start:1294
Info====> pop :<dia:connections1>	 start:1294 end:1399
Info====> pop :<dia:object1>	 start:287 end:1419
Info====> push:<dia:object2>	 start:1426
Info====> push:<dia:attribute10>	 start:1490
Info====> pop :<dia:attribute10>	 start:1490 end:1584
Info====> push:<dia:attribute11>	 start:1593
Info====> pop :<dia:attribute11>	 start:1593 end:1709
Info====> push:<dia:attribute12>	 start:1718
Info====> push:<dia:composite>	 start:1757
Info====> push:<dia:attribute13>	 start:1796
Info====> pop :<dia:attribute13>	 start:1796 end:1897
Info====> push:<dia:attribute14>	 start:1910
Info====> pop :<dia:attribute14>	 start:1910 end:2008
Info====> push:<dia:attribute14>	 start:2021
Info====> pop :<dia:attribute14>	 start:2021 end:2114
Info====> push:<dia:attribute15>	 start:2127
Info====> pop :<dia:attribute15>	 start:2127 end:2225
Info====> push:<dia:attribute16>	 start:2238
Info====> pop :<dia:attribute16>	 start:2238 end:2337
Info====> push:<dia:attribute17>	 start:2350
Info====> pop :<dia:attribute17>	 start:2350 end:2446
Info====> pop :<dia:composite>	 start:1757 end:2472
Info====> pop :<dia:attribute12>	 start:1718 end:2498
Info====> pop :<dia:object2>	 start:1426 end:2518
Info====> push:<dia:object3>	 start:2525
Info====> push:<dia:attribute16>	 start:2588
Info====> pop :<dia:attribute16>	 start:2588 end:2683
Info====> push:<dia:attribute17>	 start:2692
Info====> pop :<dia:attribute17>	 start:2692 end:2801
Info====> push:<dia:attribute18>	 start:2810
Info====> pop :<dia:attribute18>	 start:2810 end:2909
Info====> push:<dia:attribute19>	 start:2918
Info====> pop :<dia:attribute19>	 start:2918 end:3010
Info====> push:<dia:attribute20>	 start:3019
Info====> pop :<dia:attribute20>	 start:3019 end:3109
Info====> push:<dia:attribute21>	 start:3118
Info====> pop :<dia:attribute21>	 start:3118 end:3211
Info====> push:<dia:attribute22>	 start:3220
Info====> pop :<dia:attribute22>	 start:3220 end:3318
Info====> push:<dia:attribute23>	 start:3327
Info====> pop :<dia:attribute23>	 start:3327 end:3424
Info====> push:<dia:attribute24>	 start:3433
Info====> pop :<dia:attribute24>	 start:3433 end:3522
Info====> pop :<dia:object3>	 start:2525 end:3542
Info====> pop :<dia:layer>	 start:237 end:3558
Info====> pop :<dia:diagram>	 start:0 end:3574
			2019-01-15 13:20:27
			2019-01-15 13:20:27
########################## [ step 2 ] ##########################
============== [ step3 ./test3.xml valid:[1] step3 ] ===========

[yangyu@VM_0_3_centos xml_parser]$ 
[yangyu@VM_0_3_centos xml_parser]$ ./xml ./debug.xml 
========================== [ BEGIN  ] ==========================
########################## [ step 1 ] ##########################
			2019-01-15 13:20:37
Error===> [./debug.xml]:'>'  in 197 without '<'
[yangyu@VM_0_3_centos xml_parser]$ 
```
