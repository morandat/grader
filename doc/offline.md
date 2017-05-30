# Instructions pour les enseignants

## Ajouter un exercice

### Créer un exercice

à la racine des exercices :

`make EXO=mon_super_exo new`

Editez les fichiers crées dans le répertoire `mon_super_exo`, et n'oubliez pas les ajouter à git (`git add mon_super_exo`).

### Tester les exercices localement

à la racine des exercices
: - Pour compiler : `make EXO=nomexo`
 - Pour nettoyer : `make EXO=nomexo clean`

depuis un répertoire de travail, i.e., contenant le Makefile étudiant
: - Pour compiler : `make -C $PATH_TO_REPO EXO=nomexo`
 - Pour nettoyer : `make -C $PATH_TO_REPO EXO=nomexo clean`
 - Pour tester une correction : ``make -C $PATH_TO_REPO EXO=nomexo DST=`pwd`/nomsource_sans_extension compile``

### Tester les exercices en ligne

- **EXT** Ouvrir un proxy dans un autre terminal : `ssh -D 1234 ssh.enseirb-matmeca.fr -N` (le -N permet de ne pas créer de shell, je vous laisse lire la doc pour l'ouvrir en tâche de fond)
- Utiliser la version enseignant : `make PORT=4444  LOGIN=votrelogin commande_etudiante` (**EXT** `PROXY=socks5://localhost:1234`)
- Vous pouvez aussi vous logguer : ``eval `make login USERNAME=votre_login PORT=4444 PROXY=socks5://localhost:1234` `` pour se délogguer `unset LOGIN PORT PROXY` (ou fermez simplement votre terminal)

### Changer le code source

- Clonez : `git -c http.sslverify=false clone https://votrelogin@thor.enseirb-matmeca.fr:4444/git/cexos.git`  (voir ci-après si vous êtes à l'extérieur)
- Autorisez la connection : `git config --add --local http.sslverify false`
- Faites vos modifs
- Committez/poussez (les profs y auront accès)
- Testez (cf. plus haut)
- Si vous êtes content : poussez sur la branche stable (certains droits sont requis)

#### De l'extérieur

On réapplique le truc du proxy (`ssh -D 1234 ssh.enseirb-matmeca.fr -N`)

~~~
git -c http.proxy=socks5://localhost:1234 clone https://votrelogin@thor.enseirb-matmeca.fr:4444/git/cexos.git 
cd cexos.git
git config --add --local http.proxy socks5://localhost:1234
~~~

### Passer les exercices en version stable

Rajoutez une strategie de merge trivialle dans votre répo (à faire une fois) :

~~~
git config --add --local merge.ours.driver true
# --local peut être remplacé par --global sans aucune crainte
~~~

Merger la branche master dans la branche stable :

~~~
# On travaille sur la branche stable
git checkout stable
# On merge (on peut aussi "cherry pick"er certains commits)
git merge master
# On publie
git push
# On se remet sur la branche master avant de faire des bétises
git checkout master
~~~
