#ifndef IPV4INPUT_H
#define IPV4INPUT_H

#include <QLineEdit>
#include <QValidator>

class Ipv4OctetValidator : public QValidator
{
public:
    explicit Ipv4OctetValidator(QObject *parent) : QValidator(parent) {}

    State validate(QString &input, int &position) const override
    {
        Q_UNUSED(position)
        const QStringList octets = input.split('.');
        if(octets.size() != 4) return Invalid;
        bool complete = true;
        for(QString octet : octets)
        {
            octet.remove(QChar(0x2800));
            if(octet.isEmpty()) { complete = false; continue; }
            bool numeric = false;
            const int value = octet.toInt(&numeric);
            if(!numeric || octet.size() > 3 || value > 255) return Invalid;
        }
        return complete ? Acceptable : Intermediate;
    }
};

inline void configureIpv4Input(QLineEdit *edit)
{
    edit->setInputMask(QStringLiteral("000.000.000.000;") + QChar(0x2800));
    edit->setValidator(new Ipv4OctetValidator(edit));
}

#endif
