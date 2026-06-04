#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include "CreateYamlFile.h"

const String configYaml="/config.yaml";

void writeYAMLFile(String yamlContent) {
  // Open or create a file on SPIFFS in write mode
  File file = SPIFFS.open(configYaml, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  file.print(yamlContent);
  // Close the file
  file.close();
}

String readYAML(){
  File file = SPIFFS.open(configYaml, FILE_READ);
  String yamlContent = file.readString();
  file.close();
  return yamlContent;
}

String ReadYmlUsingSPIFFS(String DefaultYamlContent) {
  if (!SPIFFS.begin(true)) {
      Serial.println("Failed to initialize SPIFFS!");
      return "0";
  }
  Serial.println("SPIFFS initialized successfully!");
  /*
  /////////////// in case you want to remove the yaml file and rewritie it
  if (SPIFFS.exists(configYaml)) {
        Serial.print("Deleting file: ");
        Serial.println(configYaml);

        // Delete the file
        if (SPIFFS.remove(configYaml)) {
            Serial.println("File deleted successfully");
        } else {
            Serial.println("Failed to delete the file");
        }
    } else {
        Serial.print("File does not exist: ");
        Serial.println(configYaml);
  }
  */
  /////////////
  if ( SPIFFS.exists(configYaml)) {
    Serial.println("File "+ configYaml +" found!");
  } else {
    Serial.println("Failed to fine config file, writing new " +configYaml+ " file!");
  // Create and write to a file
    writeYAMLFile(DefaultYamlContent);
  } 
  return readYAML();
}
