#pragma once
#include "ui/common/display_base_types.h"
#include "processor/registers.h"
#include <QAbstractTableModel>
#include <memory>

namespace Kites
{

/**
 * @brief This class represents a model for displayin and
 * managing the register file in a tabular format.
 *
 */
class RegisterModel : public QAbstractTableModel
{
    Q_OBJECT
  public:
    explicit RegisterModel(QObject *parent = nullptr, RegisterFile *regfile = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    void setDisplayBase(Base base);
    void changeRegisterFile(RegisterFile *regfile);

  private:
    RegisterFile *m_currentRegisterFile;
    Base m_displayBase = Base::Hexadecimal;
    size_t m_highlightedRegisterIndex = -1; // No register highlighted by default
  public slots:
    void updateRegisterValue(size_t regIndex, uint64_t value);
    void registerResetSlot();
    void updateFRegisterValue(size_t regIndex, uint64_t value);
    void updatePRegisterValue(size_t regIndex, uint16_t value);
};

} // namespace Kites
