package dava_framework_EditorsTmp_2_ResourceEditor.buildTypes

import jetbrains.buildServer.configs.kotlin.v10.*
import jetbrains.buildServer.configs.kotlin.v10.buildSteps.ScriptBuildStep
import jetbrains.buildServer.configs.kotlin.v10.buildSteps.ScriptBuildStep.*
import jetbrains.buildServer.configs.kotlin.v10.buildSteps.script

object dava_framework_EditorsTmp_2_ResourceEditor_ResourceEditorMac : BuildType({
    template = "dava_framework_TemplateDAVATools_mac"
    uuid = "24480e8f-24ae-4335-a24c-0b43447c8a93"
    extId = "dava_framework_EditorsTmp_2_ResourceEditor_ResourceEditorMac"
    name = "ResourceEditor_mac"

    params {
        param("add_definitions", "-DQT_VERSION=%QT_VERSION%,-DUNITY_BUILD=%UNITY_BUILD%,-DDEPLOY=true,-DCUSTOM_DAVA_CONFIG_PATH_MAC=%DavaConfigMac%")
        param("appID", "RE")
        param("pathToProject", "%system.teamcity.build.checkoutDir%/dava.framework/Programs/%ProjectName%")
        param("ProjectName", "ResourceEditor")
    }

    steps {
        script {
            name = "SelfTest"
            id = "RUNNER_954"
            enabled = false
            workingDir = "%pathToProjectApp%"
            scriptContent = "./ResourceEditor.app/Contents/MacOS/ResourceEditor --selftest"
        }
        stepsOrder = arrayListOf("RUNNER_1085", "RUNNER_132", "RUNNER_133", "RUNNER_134", "RUNNER_135", "RUNNER_954")
    }
})
