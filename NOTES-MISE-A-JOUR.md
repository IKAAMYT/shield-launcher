# AlterBO4 — fusion de la mise à jour upstream (shield-launcher 1.1.1)

Fusion faite le 2026-10-04 depuis le dossier "BO4 Update" (NotNierPea/shield-launcher).
Toutes les personnalisations AlterCOD sont conservées (thème or/noir, textes FR, Discord RPC,
polices embarquées, icônes, updater sur IKAAMYT/shield-launcher, project-bo4.json avec ton IP).

## Repris d'upstream
- source/launcher/launcher_funcs/dll_loading.cpp / .hpp
  - vérification CRC-32 : XInput9_1_0.dll n'est plus ré-extrait s'il est déjà à jour
  - nouvelle fonction extractExe() : extrait BlackOps4.exe depuis project-bo4/launcher/exe.zip
- source/launcher/launcher_main/launcher_main_window.cpp
  - startGame() appelle maintenant extractExe() après extractDlls() (gestion d'erreur commune)
  - bonus : l'écran de chargement AlterCOD est retiré si le lancement échoue
- premake5.lua (nouvelle syntaxe incrementallink/minimalrebuild/…) + tools/premake5.exe à jour
- project-bo4/Latest-Release-Files/project-bo4/zone/support.ff (nouveau client)
- project-bo4/Latest-Release-Files/project-bo4/plugins/acts-shield-plugin.dll (nouveau client)
- project-bo4/Latest-Release-Files/project-bo4/zone_mods/T8ShieldSupport_FF/config.json
  (nouveau format "fastfiles": { "support": { "type": "common" } })
- generate.bat : ligne `git submodule update` d'upstream ajoutée, mais on reste sur vs2022
  (upstream est passé à vs2026 ; le workflow GitHub tourne sur windows-2022)

## Volontairement NON repris
- deps/premake/qt6.lua : upstream a toujours la coquille QtNNetwork, notre correction est gardée
- style.cpp, settings_dialog.cpp, resources.rc, project-bo4.py, icônes, launcher.png : thème AlterCOD
- auto_update.cpp : version 3.0.0 + repo IKAAMYT + textes FR + script batch robuste gardés
- project-bo4.json : ton IP / nom gardés (upstream remet des valeurs par défaut)
- Shield_Launcher.exe et "Launch Project BO4.exe" dans Latest-Release-Files : ce sont les binaires
  upstream (non rebrandés). À REGÉNÉRER via le workflow GitHub Actions après cette fusion.

## À FAIRE — IMPORTANT
1. Le nouveau launcher exige **project-bo4/launcher/exe.zip** (contient BlackOps4.exe patché).
   Il n'est ni dans "BO4 Update" ni dans AlterBO4 (les .zip ne sont pas dans le repo upstream).
   Récupère exe.zip, mp.zip et solo.zip depuis la release upstream :
   https://github.com/NotNierPea/shield-launcher/releases/tag/Release
   et mets-les dans project-bo4/Latest-Release-Files/project-bo4/launcher/
   Sans exe.zip, le bouton Online/Offline affichera "Make sure exe.zip exists".
2. Recompiler le launcher (push sur GitHub -> Actions) puis remplacer Shield_Launcher.exe.
3. project-bo4/Latest-Release-Files/project-bo4/internals/T8ShieldSupport/ n'existe plus chez
   upstream. Gardé ici par prudence ; à supprimer si le nouveau client ne s'en sert plus.
4. FAIT : version passée à 3.1.0 (auto_update.cpp), launcher recompilé (build #72),
   release BROUILLON "3.1.0" créée sur GitHub avec AlterBO4_Update_3.1.0.zip.
   → Pour la diffuser aux joueurs : GitHub > Releases > "AlterBO4 Launcher 3.1.0" > Edit > Publish release.

## COMMENT PUBLIER UNE PROCHAINE VERSION (ex : 3.2.0)
1. Dans source/launcher/launcher_funcs/auto_update.cpp, mets SERVER_VERSION = "3.2.0".
2. Pousse sur GitHub : le build tourne tout seul et dépose Shield_Launcher.exe dans une
   release brouillon "build-N" (Releases > Drafts). Télécharge-le et mets-le dans
   project-bo4/Latest-Release-Files/project-bo4/launcher/, puis pousse à nouveau.
3. Onglet Actions > "Publier une release AlterBO4" > Run workflow > version = 3.2.0.
   Le workflow assemble AlterBO4_Update_3.2.0.zip (exe.zip pris chez upstream) et crée la
   release brouillon. Vérifie, puis "Publish release" : l'updater des joueurs la propose.
