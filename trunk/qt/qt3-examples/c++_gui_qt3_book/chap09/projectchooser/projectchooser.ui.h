void ProjectChooser::init()
{
    rightButton->setPixmap(
            QPixmap::fromMimeSource("rightarrow.png"));
    leftButton->setPixmap(
            QPixmap::fromMimeSource("leftarrow.png"));

    projectAView->insertItem("Sally Prudhomme");
    projectAView->insertItem("Henryk Sienkiewicz");
    projectAView->insertItem("Selma Lagerl�f");
    projectAView->insertItem("Rabindranath Tagore");
    projectAView->insertItem("Carl Spitteler");

    projectBView->insertItem("Eugene O'Neill");
    projectBView->insertItem("Bj�rnstjerne Bj�rnson");
    projectBView->insertItem("Anatole France");
}

void ProjectChooser::move(ProjectView *fromProject,
                          ProjectView *toProject)
{
    QString person = fromProject->currentText();
    if (!person.isEmpty()) {
        fromProject->removeItem(fromProject->currentItem());
        toProject->insertItem(person);
    }
}

void ProjectChooser::moveToTheRight()
{
    move(projectAView, projectBView);
}

void ProjectChooser::moveToTheLeft()
{
    move(projectBView, projectAView);
}
