using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;

namespace RenderMax.CaptureReplay
{
    public static class CaptureReplayOrderMenu
    {
        [MenuItem("Tools/RenderMax/Advanced/Replay Order/Enable EID Ordered Geometry")]
        static void Enable() { Set(true); }
        [MenuItem("Tools/RenderMax/Advanced/Replay Order/Restore Normal Unity Sorting")]
        static void Disable() { Set(false); }
        static void Set(bool ordered)
        {
            var controllers = Object.FindObjectsOfType<CaptureReplayController>();
            if (controllers.Length != 1)
            {
                EditorUtility.DisplayDialog("RenderMax replay order", "Open a scene with exactly one CaptureReplayController. Found: " + controllers.Length, "OK");
                return;
            }
            var controller = controllers[0];
            Undo.RecordObject(controller, "Change RenderMax replay order");
            controller.orderedCommandBufferReplay = ordered;
            controller.Rebuild();
            EditorUtility.SetDirty(controller);
            EditorSceneManager.MarkSceneDirty(controller.gameObject.scene);
            Selection.activeGameObject = controller.gameObject;
            SceneView.RepaintAll();
            Debug.Log(controller.orderedStatus, controller);
        }
    }
}
