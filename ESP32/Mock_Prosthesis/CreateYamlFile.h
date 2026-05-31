
#ifndef CREATE_YAML_H
#define CREATE_YAML_H

#include <FS.h>
#include <SPIFFS.h>
#include <Arduino.h>


extern const String configYaml;

void writeYAMLFile(String yamlContent);
String readYAML();
String ReadYmlUsingSPIFFS(String DefaultYamlContent);

#endif //CREATE_YAML_H