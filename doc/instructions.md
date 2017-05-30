# Instructions

La plate-forme d'exercice est compatible avec tous les systèmes d'exploitations majeur.
Elle ne requiert que `make` et `cURL` installé sur votre poste de travail (cf. section dépendances).

## Utilisation

- Télécharger ce [Makefile](/Makefile), et le copier dans votre répertoire de travail (`curl -k --create-dirs -owork/Makefile https://thor.enseirb-matmeca.fr:4443/Makefile`).
- Lister les exercices: taper `make list` ou plus simplement `make`. La liste est aussi disponible en ligne depuis la [page principale](/).
- Récupérer un exercice: taper `make get-XXX` où `XXX` est le nom de l'exercice. Il est aussi possible de les télécharger depuis la [page principale](/).
- Faire l'exercice.
- Tester sa validité: `make test-XXX` où `XXX` est le nom de l'exercice (votre fichier `.c` sans son extension).

## Plus de commandes

Pour tester votre `main`
: `make main-XXXX ARGS=""` (les arguments entre "guillemets" seront passés tels quels)

Réinstaller un exercice
: `make get-XXX FORCE=1`

Nettoyer un exercice
: `make clean-XXX`

## Dépendences

- `make`
- `cURL`

MacOSX
: `cURL` est déjà installé par défaut. `make` devrait l'être aussi. Sinon il faut installer `XCode` et activer les outils lignes de commandes.

Windows
: Installer `Gnu make`  <http://gnuwin32.sourceforge.net/packages/make.htm> et `cURL` <http://curl.haxx.se/download.html>. Assurez vous qu'ils soient dans votre `PATH`.

Linux/Unix
: Il y a trop de distributions pour les lister toutes. La plupart installent ces outils par défaut. Pour Debian/Ubuntu installer le paquet `build-essential` qui vous installera tous les outils de dev. Pour toutes les distributions les paquets/ports/etc se nomment généralement `make` et `curl`.
