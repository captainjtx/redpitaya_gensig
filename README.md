# redpitaya_gensig
custom signal generator in redpitaya

To compile in ubuntu 
make CROSS_COMPILE=arm-linux-gnueabi- clean all

Important Notes:

1) The custom signal must be named "gen_ch1.csv"
2) The number of samples in "gen_ch1.csv" must be integer multiples of 16384
	(i.e. n*16384 , where n ranges from 1 to 30)
3) The IP address of your computer should be set to 192.168.1.101
	the IP address of RedPitaya is 192.168.1.100
4) Ensure that your custom signal, "gen_ch1.csv", is located in the same file as 
	as the executable on Redpitaya
