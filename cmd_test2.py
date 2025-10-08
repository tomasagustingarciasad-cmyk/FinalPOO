#!/usr/bin/env python
# -*- coding: utf-8 -*-
from cmd import Cmd 

class CLIbasico(Cmd):
    """Ejemplo basico de Interprete de Comandos"""
    doc_header = "Ayuda de comandos documentados"
    undoc_header = "Ayuda de comandos no documentados"
    ruler = "*"       
    lista = ['python', 'c++', 'php', 'java', 'c#', 'js']
            
    def do_list(self, args):
        """list palabra: Lista todos los elementos a partir de la palabra indicada"""
        argumentos = args.split()
        if len(argumentos) >= 1:
            try:
                i = self.lista.index(argumentos[0])
                print(self.lista[i:])
            except:
                print(argumentos[0] + ' no se encuentra en la lista')
        else:
            print(self.lista)

    def do_add(self, args):
        """add palabra: Agrega la palabra indicada a la lista"""
        argumentos = args.split()
        if len(argumentos) >= 1:
            self.lista.append(argumentos[0])
        else:
            self.onecmd('help add')
        
    def do_quit(self, args):
        """quit sale del interprete"""
        print("Ejecucion UnCLI terminada")
        raise SystemExit
    
    def default(self, args):
        print("Error. El comando \'" + args + "\' no esta disponible")

    def precmd(self, args):
        args = args.lower()
        return(args)        

if __name__ == '__main__':
    uncli = CLIbasico()
    uncli.prompt = ';-) '
    uncli.cmdloop('Iniciando entrada de comandos...')

