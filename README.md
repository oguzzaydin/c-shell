


* Temel komutlar: `exit`, `pwd`, `clear` ve `cd`
* Çevre yönetimi  `setenv` ve `unsetenv`
* Fork ve alt prosseslerle program çağırma
* I/O yönlendirme (use of `dup2` system call) aşağıdakilerle sınırlı
        `<cmd> <args> > <output>`  
        `<cmd> <args> < <input> > <output>`
* Programların arka planda yürütülmesi `&`
* Piping implemented (`<cmd1> | <cmd2>`) üzerinden `pipe` ve `dup2` syscalls. Çoklu pipe kullanımına izin verir.
* SIGINT signal ne zaman Ctrl-C basıldığında kabuktan çıkılmaz. 
