cmd_/workspace/src/03_pilote/07_pilote/modules.order := {   echo /workspace/src/03_pilote/07_pilote/locking.ko; :; } | awk '!x[$$0]++' - > /workspace/src/03_pilote/07_pilote/modules.order
