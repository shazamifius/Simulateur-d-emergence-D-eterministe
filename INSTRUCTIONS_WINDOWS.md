# Guide d'Installation pour Windows (SED)

Ce guide vous explique comment installer les outils nécessaires pour compiler et exécuter le projet SED (Simulateur d'Émergence Déterministe) sur un système d'exploitation Windows.

L'erreur `make n'est pas reconnu` que vous avez rencontrée est normale : `make` et le compilateur `g++` ne sont pas inclus par défaut dans Windows. La solution la plus simple et la plus propre est d'utiliser **MSYS2**, qui fournit un environnement de type Linux directement sur Windows.

Suivez ces étapes dans l'ordre.

### Étape 1 : Installer MSYS2

MSYS2 est un logiciel qui nous donnera accès à un terminal et à un gestionnaire de paquets (similaire à ce qu'on trouve sur Linux) pour installer nos outils.

1.  **Téléchargez l'installeur :**
    *   Allez sur le site officiel de MSYS2 : [https://www.msys2.org/](https://www.msys2.org/)
    *   Cliquez sur le gros bouton de téléchargement pour obtenir le fichier `msys2-x86_64-....exe`.

2.  **Lancez l'installation :**
    *   Exécutez le fichier que vous venez de télécharger.
    *   Suivez les instructions d'installation. Vous pouvez laisser le dossier d'installation par défaut (`C:\msys64`).

### Étape 2 : Installer les Outils de Compilation (g++, make)

Une fois l'installation terminée, une fenêtre de terminal MSYS2 devrait s'ouvrir. Si ce n'est pas le cas, cherchez "MSYS2 MINGW64" dans votre menu Démarrer et lancez-le.

1.  **Mettez à jour le système :**
    *   Dans le terminal MSYS2, tapez la commande suivante et appuyez sur `Entrée`. Cette commande met à jour la base de données des paquets.
    ```bash
    pacman -Syu
    ```
    *   Il se peut qu'on vous demande de fermer la fenêtre du terminal. Si c'est le cas, fermez-la, relancez "MSYS2 MINGW64" et tapez à nouveau la même commande `pacman -Syu` pour finaliser la mise à jour.

2.  **Installez les outils :**
    *   Maintenant, tapez la commande suivante pour installer `g++`, `make`, et le débogueur `gdb` en une seule fois. C'est le "toolchain" MinGW.
    ```bash
    pacman -S --needed base-devel mingw-w64-x86_64-toolchain
    ```
    *   Lorsque le système vous demande quels paquets installer, appuyez simplement sur `Entrée` pour tout sélectionner par défaut.
    *   Confirmez l'installation en tapant `O` (ou `Y` si votre système est en anglais) lorsque cela vous est demandé.

### Étape 3 : Ajouter les Outils à l'Environnement Windows (le PATH)

C'est l'étape la plus importante. Elle permet à VS Code et à d'autres programmes de trouver les outils que vous venez d'installer.

1.  **Ouvrez les Paramètres de l'Environnement :**
    *   Appuyez sur la touche `Windows` et tapez `env`.
    *   Cliquez sur "Modifier les variables d'environnement pour votre compte".

2.  **Modifiez la variable `Path` :**
    *   Dans la section "Variables utilisateur", sélectionnez la variable `Path` et cliquez sur "Modifier".
    *   Cliquez sur "Nouveau".
    *   Ajoutez le chemin suivant (si vous avez laissé le dossier d'installation par défaut) :
        `C:\msys64\mingw64\bin`
    *   Cliquez sur "OK" pour fermer toutes les fenêtres.

3.  **Vérifiez l'installation :**
    *   **Fermez toutes les fenêtres de terminal et de VS Code que vous aviez ouvertes.** C'est crucial pour que les changements soient pris en compte.
    *   Ouvrez un nouveau terminal (PowerShell, CMD, ou le terminal de VS Code).
    *   Tapez les commandes suivantes. Chacune doit afficher une version, et non une erreur :
        ```bash
        g++ --version
        make --version
        gdb --version
        ```

### Étape 4 : Compiler et Exécuter dans VS Code

Vous êtes prêt !

1.  Ouvrez le dossier du projet SED dans VS Code.
2.  Utilisez le raccourci `Ctrl+Shift+B` (ou `Terminal > Run Build Task...`). VS Code utilisera automatiquement le `Makefile` pour compiler le projet, car `make` est maintenant reconnu par le système.
3.  Pour lancer le programme, ouvrez un terminal dans VS Code et tapez :
    `./sed_simulator.exe`
4.  Pour déboguer, appuyez simplement sur `F5`.

Vous avez maintenant un environnement de développement C++ complet et fonctionnel sur Windows. Toutes les actions dans VS Code fonctionneront comme prévu.