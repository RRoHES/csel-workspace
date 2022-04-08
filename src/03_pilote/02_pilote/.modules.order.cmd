cmd_/workspace/src/03_pilote/02_pilote/modules.order := {   echo /workspace/src/03_pilote/02_pilote/module_car.ko; :; } | awk '!x[$$0]++' - > /workspace/src/03_pilote/02_pilote/modules.order
