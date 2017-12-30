
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <termios.h>
#include "kabuk.h"

#define LIMIT 256  //Bir komut için simge sayısı
#define MAXSATIR 1024 //Kullanıcı girdisinin azami karakter sayısı


void init(){
		
        GBSH_PID = getpid();
          //kabuk eğer STDIN terminal ise interaktiftir.
        GBSH_IS_INTERACTIVE = isatty(STDIN_FILENO);  

		if (GBSH_IS_INTERACTIVE) {
			
			while (tcgetpgrp(STDIN_FILENO) != (GBSH_PGID = getpgrp()))
					kill(GBSH_PID, SIGTTIN);             
	              
	              
	        //SIGCHILD ve SIGINT için sinyal işleyiciler ayarlanır
			act_child.sa_handler = signalHandler_child;
			act_int.sa_handler = signalHandler_int;			
			
			
			
			sigaction(SIGCHLD, &act_child, 0);
			sigaction(SIGINT, &act_int, 0);
			
			
			setpgid(GBSH_PID, GBSH_PID); 
			GBSH_PGID = getpgrp();
			if (GBSH_PID != GBSH_PGID) {
					printf("Hata, kabuk süreç grubu lideri değil!");
					exit(EXIT_FAILURE);
			}
			
			tcsetpgrp(STDIN_FILENO, GBSH_PGID);  
			
			//Kabuk için varsayılan terminal özelliklerini kaydet
			tcgetattr(STDIN_FILENO, &GBSH_TMODES);

			
			currentDirectory = (char*) calloc(1024, sizeof(char));
        } else {
                printf("Kabuk etkileşimli hale getirilemedi.\n");
                exit(EXIT_FAILURE);
        }
}


void welcomeScreen(){

        printf("\t-----Kabuğa Hoşgeldiniz-----\n");
	printf("\n\n");
}


void signalHandler_child(int p){
	
	int durum, cocuk_degeri,chpid;
	chpid = waitpid(-1, &durum, WNOHANG);
	if (chpid > 0)
	{
		if (WIFEXITED(durum))
	    {
	        cocuk_degeri = WEXITSTATUS(durum);
	        printf("[%d] retval : %d \n",chpid, cocuk_degeri);
	    }
	}
}

void signalHandler_int(int p){
	//Çocuk prossese bir SIGTERM sinyali gönderiyoruz
	if (kill(pid,SIGTERM) == 0){
		printf("\n%d işlemi SIGINT sinyali aldı.\n",pid);
		no_reprint_prmpt = 1;			
	}else{
		printf("\n");
	}
}

//Kabuk için istemi görüntüler
void shellPrompt(){
	// Komut istemini "<user> @ <host><cwd> >" şeklinde yazdık
 	char hostn[1204] = "";
	gethostname(hostn, sizeof(hostn));
	printf("%s@%s %s > ", getenv("LOGNAME"), hostn, getcwd(currentDirectory, 1024));
}


int changeDirectory(char* args[]){
	//Hiçbir yol yazmazsak (sadece 'cd') ev dizinine gidelim
	if (args[1] == NULL) {
		chdir(getenv("HOME")); 
		return 1;
	}
	
	else{ 
		if (chdir(args[1]) == -1) {
			printf(" %s: böyle bir rehber yok\n", args[1]);
            return -1;
		}
	}
	return 0;
}

//Farkli çevre değişkenlerini yönetmek için kullanılan bir yontem
 
int manageEnviron(char * args[], int secenek){
	char **env_aux;
	switch(secenek){
		
		case 0: 
			for(env_aux = environ; *env_aux != 0; env_aux ++){
				printf("%s\n", *env_aux);
			}
			break;
		
		case 1: 
			if((args[1] == NULL) && args[2] == NULL){
				printf("%s","Yetersiz Giriş Argümanı\n");
				return -1;
			}
			
			
			if(getenv(args[1]) != NULL){
				printf("%s", "Değişken üzerine yazılmış.\n");
			}else{
				printf("%s", "Değişken oluşturuldu.\n");
			}
			
			
			if (args[2] == NULL){
				setenv(args[1], "", 1);
			
			}else{
				setenv(args[1], args[2], 1);
			}
			break;
		
		case 2:
			if(args[1] == NULL){
				printf("%s","Yetersiz Giriş Argümanı\n");
				return -1;
			}
			if(getenv(args[1]) != NULL){
				unsetenv(args[1]);
				printf("%s", "Değişken Silindi.\n");
			}else{
				printf("%s", "Değişken Yok.\n");
			}
		break;
			
			
	}
	return 0;
}
 //Bir programı başlatma yönetimi Arka planda çalıştırma veya önplanda çalıştırma

void launchProg(char **args, int arkaplan){	 
	 int hata = -1;
	 
	 if((pid=fork())==-1){
		 printf("Çocuk işlemi oluşturulamadı.\n");
		 return;
	 }
	 
	if(pid==0){
		//Çocuğu SIGINT sinyallerini yoksayacak şekilde ayarladık.
			
		signal(SIGINT, SIG_IGN);
		
		setenv("parent",getcwd(currentDirectory, 1024),1);	
		
		
		if (execvp(args[0],args)==hata){
			printf("Komut bulunamadı.");
			kill(getpid(),SIGTERM);
		}
	 }
//Aşağıdakiler ebeveyn tarafından yürütülecektir.


	 //Sürecin arka planda olması istenmiyorsa çocugun bitmesi beklenir.
	 
	 if (arkaplan == 0){
		 waitpid(pid,NULL,0);
	 }else{
		
		 printf("PID ile oluşturulmuş işlem: %d\n",pid);
	 }	 
}
 
 // G / Ç yönlendirme işlemini yönetmek için kullanılan yöntem
void fileIO(char * args[], char* girisDosyasi, char* cikisDosyasi, int secenek){
	 
	int hata = -1;
	
	int dosyaTanimlayici; 
	
	if((pid=fork())==-1){
		printf("Çocuk işlemi oluşturulamadı.\n");
		return;
	}
	if(pid==0){
		
		if (secenek == 0){
			//Output redirection
			dosyaTanimlayici = open(cikisDosyasi, O_CREAT | O_TRUNC | O_WRONLY, 0600); 
			
			dup2(dosyaTanimlayici, STDOUT_FILENO); 
			close(dosyaTanimlayici);
		
		}else if (secenek == 1){
			
			dosyaTanimlayici=open(girisDosyasi, O_RDONLY, 0600);
			dup2(dosyaTanimlayici,STDIN_FILENO);//standart girdi yonlendiriliyor.
			close(dosyaTanimlayici); 
		}
		 
		setenv("parent",getcwd(currentDirectory, 1024),1);
		
		if (execvp(args[0],args)==hata){
			printf("hata");
			kill(getpid(),SIGTERM);
		}		 
	}
	waitpid(pid,NULL,0);
}

 // Pipe ları yönetmek için kullanılan yöntem.
void pipeHandler(char * args[]){
	
	int dosyaTanitici[2]; 
	int dosyaTanitici2[2];
	
	int sayi_komutlari = 0;
	
	char *komut[256];
	
	pid_t pid;
	
	int hata = -1;
	int son = 0;
	
	//Farklı döngüler için kullanılan değişkenler
	int i = 0;
	int j = 0;
	int k = 0;
	int l = 0;
	
//Önce komut sayısı hesaplanır 

	
	while (args[l] != NULL){
		if (strcmp(args[l],"|") == 0){
			sayi_komutlari++;
		}
		l++;
	}
	sayi_komutlari++;
	
	/* Bu yöntemin ana döngüsü. '|' 
	Arasındaki her komut için pipe lar yapılandırılacak
	ve standart giriş ve / veya çıkış değiştirilecektir.
	Sonra idam edilecek.*/
	
	while (args[j] != NULL && son != 1){
		k = 0;
		/* Her yinelemede yürütülecek komutu saklamak için 
		yardımcı bir dizi işaretçi kullanıyoruz.*/
		while (strcmp(args[j],"|") != 0){
			komut[k] = args[j];
			j++;	
			if (args[j] == NULL){
				/* 'son' değişkeni, hiçbir argüman bulunmadığında, 
				programın tekrar döngüye girmesini önler. */
				son = 1;
				k++;
				break;
			}
			k++;
		}
		// Komutun son konumu, onu exec işlevine 
		// geçirirken biti olduğunu belirtmek için NULL olacaktır.
		komut[k] = NULL;
		j++;		
		
		// Yinelemeyle mi yoksa bir iterasyonda mı olduğumuza bağlı 
		// olarak, pipe girdileri ve çıktıları için farklı tanımlayıcılar
		// belirleyeceğiz. Bu şekilde, iki iterasyon arasında bir pipe 
		// paylaşılacak ve iki farklı komutun giriş ve çıkışlarını 
		//bağlayabileceğiz.
		
		if (i % 2 != 0){
			pipe(dosyaTanitici); // Tek i için
		}else{
			pipe(dosyaTanitici2); // i için bile
		}
		
		pid=fork();
		
		if(pid==-1){			
			if (i != sayi_komutlari - 1){
				if (i % 2 != 0){
					close(dosyaTanitici[1]); // Tek i için
				}else{
					close(dosyaTanitici2[1]); // i için bile 
				} 
			}			
			printf("Çocuk işlemi oluşturulamadı.\n");
			return;
		}
		if(pid==0){
			// Eğer birinci komuta gidersek.
			if (i == 0){
				dup2(dosyaTanitici2[1], STDOUT_FILENO);
			}
			/*
			Son komutu kullanırsak, bunun tek veya çift konumda 
			olup olmadığına bağlı olarak standart girdiyi bir pipe 
			veya başka bir boruyla değiştiririz. Terminal çıktısını 
			görmek istediğimiz için standart çıktıya dokunulmaz.
			*/
			else if (i == sayi_komutlari - 1){
				if (sayi_komutlari % 2 != 0){ 
					dup2(dosyaTanitici[0],STDIN_FILENO);
				}else{ // hatta birkaç komut için.
					dup2(dosyaTanitici2[0],STDIN_FILENO);
				}
			/* Eğer ortada olan bir komuta sahipsek, biri girdi için, 
			diğeri çıktı için olmak üzere iki pipe kullanmak zorunda 
			kalacağız. Hangi dosya tanımlayıcısının her girdi / çıktıya 
			karşılık geldiğini seçmek için pozisyon da önemlidir.*/
			}else{ // Tek i için
				if (i % 2 != 0){
					dup2(dosyaTanitici2[0],STDIN_FILENO); 
					dup2(dosyaTanitici[1],STDOUT_FILENO);
				}else{ // i için bile
					dup2(dosyaTanitici[0],STDIN_FILENO); 
					dup2(dosyaTanitici2[1],STDOUT_FILENO);					
				} 
			}
			
			if (execvp(komut[0],komut)==hata){
				kill(getpid(),SIGTERM);
			}		
		}
				
		// Aileye Ait Tanımlayıcıları Kapatma.
		if (i == 0){
			close(dosyaTanitici2[1]);
		}
		else if (i == sayi_komutlari - 1){
			if (sayi_komutlari % 2 != 0){					
				close(dosyaTanitici[0]);
			}else{					
				close(dosyaTanitici2[0]);
			}
		}else{
			if (i % 2 != 0){					
				close(dosyaTanitici2[0]);
				close(dosyaTanitici[1]);
			}else{					
				close(dosyaTanitici[0]);
				close(dosyaTanitici2[1]);
			}
		}
				
		waitpid(pid,NULL,0);
				
		i++;	
	}
}
			
// Standart girdi yoluyla girilen komutları işlemek için kullanılan yöntem.
int commandHandler(char * args[]){
	int i = 0;
	int j = 0;
	
	int dosyaTanimlayici;
	int standartCikis;
	
	int arkaplan = 0;
	
	char *args_aux[256];
	
	// Özel karakterleri 
	// ararız ve argümanlar için komutu yeni bir dizi ile ayırırız.
	while ( args[j] != NULL){
		if ( (strcmp(args[j],">") == 0) || (strcmp(args[j],"<") == 0) || (strcmp(args[j],"&") == 0)){
			break;
		}
		args_aux[j] = args[j];
		j++;
	}
	// 'quit' komutu kabuktan çıkıyor.
	args_aux[j]=NULL;
	if(strcmp(args[0],"quit") == 0)
	{
		int durum;
		while (!waitpid(-1,&durum,WNOHANG)){}
		exit(0);
	}
	// 'pwd' komutu geçerli dizini yazdırır.
 	else if (strcmp(args[0],"pwd") == 0){
		if (args[j] != NULL){
			// Eğer dosya çıktısını istiyorsak.
			if ( (strcmp(args[j],">") == 0) && (args[j+1] != NULL) ){
				dosyaTanimlayici = open(args[j+1], O_CREAT | O_TRUNC | O_WRONLY, 0600); 
				// Standart çıktıyı uygun dosya ile değiştiriyoruz.
				standartCikis = dup(STDOUT_FILENO); 	// Önce stdout'un bir kopyasını çıkarırız çünkü geri isteyeceğiz.
				dup2(dosyaTanimlayici, STDOUT_FILENO); 
				close(dosyaTanimlayici);
				printf("%s\n", getcwd(currentDirectory, 1024));
				dup2(standartCikis, STDOUT_FILENO);
			}
		}else{
			printf("%s\n", getcwd(currentDirectory, 1024));
		}
	} 
 	// 'clear' komutu ekranı temizler.
	else if (strcmp(args[0],"clear") == 0) system("clear");
	// 'cd' komutu dosya değiştirir.
	else if (strcmp(args[0],"cd") == 0) changeDirectory(args);
	// 'environ' komutu ile çevre değişkenlerini listeleyebilirsiniz.
	else if (strcmp(args[0],"environ") == 0){
		if (args[j] != NULL){
			// Eğer dosya çıktısını istiyorsak.
			if ( (strcmp(args[j],">") == 0) && (args[j+1] != NULL) ){
				dosyaTanimlayici = open(args[j+1], O_CREAT | O_TRUNC | O_WRONLY, 0600); 
				// Standart çıktıyı uygun dosya ile değiştiriyoruz.
				standartCikis = dup(STDOUT_FILENO); // Önce stdout'un bir kopyasını çıkarırız çünkü geri isteyeceğiz.	
				dup2(dosyaTanimlayici, STDOUT_FILENO); 
				close(dosyaTanimlayici);
				manageEnviron(args,0);
				dup2(standartCikis, STDOUT_FILENO);
			}
		}else{
			manageEnviron(args,0);
		}
	}
	// 'setenv' komutu ile ortam değişkenlerini ayarlarız.
	else if (strcmp(args[0],"setenv") == 0) manageEnviron(args,1);
	// 'unsetenv' komutu ile ortam değişkenlerini tanımlarız.
	else if (strcmp(args[0],"unsetenv") == 0) manageEnviron(args,2);
	else{
		// Yukarıdaki komutların hiçbiri kullanılmadıysa, 
		// belirtilen programı çağıracağız. G / Ç yönlendirme, 
		// yönlendirilmiş yürütme veya arka plan yürütme isteği 
		// olup olmadığını algılamak zorundayız.
		while (args[i] != NULL && arkaplan == 0){
			// Arkaplanda yürütme istenirse ('&' son bağımsız değişkeni)
			// döngüden çıkıyoruz.
			if (strcmp(args[i],"&") == 0){
				arkaplan = 1;
			// Eğer '|' tespit edilirse, boru hattı istendi ve 
			// farklı uygulamaları idare edecek uygun yöntemi diyoruz.
			}else if (strcmp(args[i],"|") == 0){
				pipeHandler(args);
				return 1;
			// '<' Algılanırsa, Giriş ve Çıkış yönlendirmesi yaparız.
			// Önce verilen yapının doğru yapıp yapmadığına bakarız,
			// ve bu durumda uygun yöntemi çağırırız
			}
			else if (strcmp(args[i],"<") == 0)
			{
				if (args[i+1]==NULL )
				{
					printf ("Dosya bulunamadi\n");
                                        return -1;

				}
				fileIO(args_aux,args[i+1],NULL,1);
				return 1;
			}
			// Eğer '>' algılanırsa, çıktı yönlendirmesi var.
			// Önce verilen yapının doğru yapıp yapmadığına bakarız,
			// ve bu durumda uygun yöntemi çağırırız
			else if (strcmp(args[i],">") == 0){
				if (args[i+1]==NULL){
					printf("Giriş dosyası bulunamadı\n");
					return -1;
				}
				fileIO(args_aux,NULL,args[i+1],0);
				return 1;
			}
			i++;
		}
		// Programımızı yöntemimizle başlattık;
		// arkaplan yürütülmesini ister istemez
		args_aux[i] = NULL;
		launchProg(args_aux,arkaplan);
		
		
	}
return 1;
}


// Kabuğumuzun main metodu
int main(int argc, char *argv[], char ** envp) {
	char satir[MAXSATIR]; // kullanıcı girişi için arabellek
	char * jetonlar[LIMIT]; // komuttaki farklı simgeler için dizi
	int noJetonlar;
		
	no_reprint_prmpt = 0; 	// kabuğun baskısını önlemek için
							// bazı yöntemlerin ardından
	pid = -10; // pid'i mümkün olmayan bir pid'e başlatıyoruz.
	
	
	// Başlatma metodunu ve hoş geldiniz ekranını çağırırız.
	init();
	welcomeScreen();
    
    
	// Dışarıdaki char ** ortamımızı çevreye ayarladık, böylece
    // daha sonra başka yöntemlerle tedavi edebiliriz
	environ = envp;
	
	// Kabuk = <yol adı> / kabuk çocuğun ortam değişkeni olarak ayarladık.
	setenv("shell",getcwd(currentDirectory, 1024),1);
	
	// Ana döngü, burada kullanıcı girişi okunacak ve komut istemi yazdırılacaktır.
	while(TRUE){
		// Gerekirse kabuk istemini basarız.
		if (no_reprint_prmpt == 0) shellPrompt();
		no_reprint_prmpt = 0;
	
		// Satır tamponunu boşaltırız
		memset ( satir, '\0', MAXSATIR );

		// Kullanıcı girdisini bekleyelim
		fgets(satir, MAXSATIR, stdin);

		// Hiçbir şey yazılmazsa, döngü tekrar çalıştırılır
		if((jetonlar[0] = strtok(satir," \n\t")) == NULL) continue;
		
		// Girişin tüm işaretlerini okuduk ve argüman 
		// olarak commandHandler gönderdik.
		noJetonlar = 1;
		while((jetonlar[noJetonlar] = strtok(NULL, " \n\t")) != NULL) noJetonlar++;
		
		commandHandler(jetonlar);
		
	}          

	exit(0);
}
