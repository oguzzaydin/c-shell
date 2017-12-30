
/*

Grup No:5  
şube   :1-B
-----------------------------
b140910042 Oğuzhan AYDIN    -
b140910054 Ozan Arif ÇAKIR  -
b140910051 Mustafa YAYLALI  -
b141210054 İsmail DENİZ     -
b140910048 Onur KARAKUŞ     -
-----------------------------
*/

* Temel komutlar: `exit`, `pwd`, `clear` ve `cd`
* Çevre yönetimi  `setenv` ve `unsetenv`
* Fork ve alt prosseslerle program çağırma
* I/O yönlendirme (use of `dup2` system call) aşağıdakilerle sınırlı
        `<cmd> <args> > <output>`  
        `<cmd> <args> < <input> > <output>`
* Programların arka planda yürütülmesi `&`
* Piping implemented (`<cmd1> | <cmd2>`) üzerinden `pipe` ve `dup2` syscalls. Çoklu pipe kullanımına izin verir.
* SIGINT signal ne zaman Ctrl-C basıldığında kabuktan çıkılmaz. 
