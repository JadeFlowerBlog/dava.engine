#include "Modules/ProjectModule/ProjectModule.h"
#include "Modules/ProjectModule/ProjectData.h"
#include "Modules/LegacySupportModule/Private/Project.h"

#include "Test/TestHelpers.h"
#include "Test/ProjectModuleHelper.h"
#include "Test/DocumentsModuleHelper.h"

#include "Application/QEGlobal.h"

#include <TArc/Testing/TArcUnitTests.h>
#include <TArc/Testing/MockListener.h>
#include <TArc/DataProcessing/DataNode.h>

#include <gmock/gmock.h>

#include <QWidget>
#include <QAction>

DAVA_TARC_TESTCLASS(ProjectManagerTests)
{
    BEGIN_TESTED_MODULES()
    DECLARE_TESTED_MODULE(TestHelpers::ProjectModuleHelper)
    DECLARE_TESTED_MODULE(TestHelpers::DocumentsModuleHelper)
    DECLARE_TESTED_MODULE(ProjectModule)
    END_TESTED_MODULES()

    DAVA_TEST (OpenEmptyLastProject)
    {
        using namespace DAVA;
        using namespace TArc;
        using namespace ::testing;

        InvokeOperation(QEGlobal::OpenLastProject.ID);
        ContextAccessor* accessor = GetAccessor();
        DataContext* globalContext = accessor->GetGlobalContext();
        ProjectData* projectData = globalContext->GetData<ProjectData>();
        TEST_VERIFY(projectData == nullptr);
    }

    DAVA_TEST (CreateNewProject)
    {
        using namespace DAVA;
        using namespace TArc;
        using namespace ::testing;
        using namespace TestHelpers;

        EXPECT_CALL(*GetMockInvoker(), Invoke(QEGlobal::CloseAllDocuments.ID));

        ContextAccessor* accessor = GetAccessor();

        CreateTestProjectFolder();

        String projectPath = GetTestProjectPath().GetAbsolutePathname();
        InvokeOperation(ProjectModuleTesting::CreateProjectOperation.ID, QString::fromStdString(projectPath));

        DataContext* globalContext = accessor->GetGlobalContext();
        ProjectData* projectData = globalContext->GetData<ProjectData>();
        TEST_VERIFY(projectData != nullptr);
        TEST_VERIFY(projectData->GetUiDirectory().absolute.IsEmpty() == false);
        TEST_VERIFY(projectData->GetProjectDirectory().IsEmpty() == false);
    }

    DAVA_TEST (CloseProject)
    {
        using namespace DAVA;
        using namespace TArc;
        using namespace ::testing;
        using namespace TestHelpers;

        InvokeOperation(CreateDummyContextOperation.ID);

        EXPECT_CALL(*GetMockInvoker(), Invoke(QEGlobal::CloseAllDocuments.ID));

        QAction* action = FindActionInMenus(GetWindow(QEGlobal::windowKey), fileMenuName, closeProjectActionName);
        action->triggered();
        
        ContextAccessor* accessor = GetAccessor();
        ProjectData* data = accessor->GetGlobalContext()->GetData<ProjectData>();
        TEST_VERIFY(data == nullptr);
        TEST_VERIFY(accessor->GetContextCount() == 0);
    }

    DAVA_TEST (OpenLastProject)
    {
        using namespace DAVA;
        using namespace TArc;
        using namespace ::testing;

        wrapper = GetAccessor()->CreateWrapper(DAVA::ReflectedTypeDB::Get<ProjectData>());
        wrapper.SetListener(&listener);

        EXPECT_CALL(listener, OnDataChanged(wrapper, _))
        .WillOnce(Invoke([this](const DAVA::TArc::DataWrapper& wrapper, const DAVA::Vector<DAVA::Any>& fields)
                         {
                             TEST_VERIFY(wrapper.HasData() == true);
                             ProjectData* data = GetAccessor()->GetGlobalContext()->GetData<ProjectData>();
                             TEST_VERIFY(data != nullptr);
                             TEST_VERIFY(data->GetUiDirectory().absolute.IsEmpty() == false);
                             TEST_VERIFY(data->GetProjectDirectory().IsEmpty() == false);
                         }));

        InvokeOperation(QEGlobal::OpenLastProject.ID);
    }

    DAVA_TEST (FailCloseProject)
    {
        using namespace DAVA;
        using namespace TArc;
        using namespace ::testing;
        using namespace TestHelpers;

        InvokeOperation(CreateDummyContextOperation.ID);
        ContextAccessor* accessor = GetAccessor();
        DataContext* activeContext = accessor->GetActiveContext();
        TEST_VERIFY(activeContext != nullptr);
        SampleData* data = activeContext->GetData<SampleData>();
        data->canClose = false;

        EXPECT_CALL(*GetMockInvoker(), Invoke(QEGlobal::CloseAllDocuments.ID));

        QAction* action = FindActionInMenus(GetWindow(QEGlobal::windowKey), fileMenuName, closeProjectActionName);
        action->triggered();

        DataContext* globalContext = accessor->GetGlobalContext();
        ProjectData* projectData = globalContext->GetData<ProjectData>();
        TEST_VERIFY(projectData != nullptr);
        TEST_VERIFY(projectData->GetUiDirectory().absolute.IsEmpty() == false);
        TEST_VERIFY(projectData->GetProjectDirectory().IsEmpty() == false);
        data->canClose = true;
    }

    DAVA_TEST (CloseWindowTest)
    {
        using namespace ::testing;

        EXPECT_CALL(*GetMockInvoker(), Invoke(QEGlobal::CloseAllDocuments.ID));

        EXPECT_CALL(listener, OnDataChanged(wrapper, _))
        .WillOnce(Invoke([this](const DAVA::TArc::DataWrapper& wrapper, const DAVA::Vector<DAVA::Any>& fields)
                         {
                             TEST_VERIFY(wrapper.HasData() == false);
                             ProjectData* data = GetAccessor()->GetGlobalContext()->GetData<ProjectData>();
                             TEST_VERIFY(data == nullptr);
                             TEST_VERIFY(GetAccessor()->GetContextCount() == 0);
                         }));

        QWidget* widget = GetWindow(QEGlobal::windowKey);
        widget->close();
    }

    const QString closeProjectActionName = "Close project";
    const QString fileMenuName = "File";

    const DAVA::FilePath firstFakeProjectPath = DAVA::FilePath("~doc:/Test/ProjectManagerTest1/");

    DAVA::TArc::DataWrapper wrapper;
    DAVA::TArc::MockListener listener;
};
