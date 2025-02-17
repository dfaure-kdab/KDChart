/****************************************************************************
** Copyright (C) 2001-2019 Klaralvdalens Datakonsult AB.  All rights reserved.
**
** This file is part of the KD Chart library.
**
** Licensees holding valid commercial KD Chart licenses may use this file in
** accordance with the KD Chart Commercial License Agreement provided with
** the Software.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 and version 3 as published by the
** Free Software Foundation and appearing in the file LICENSE.GPL.txt included.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** Contact info@kdab.com if any conditions of this licensing are not
** clear to you.
**
**********************************************************************/

#include "KDChartAbstractAxis.h"
#include "KDChartAbstractAxis_p.h"
#include "KDChartAbstractDiagram.h"
#include "KDChartAbstractCartesianDiagram.h"
#include "KDChartEnums.h"
#include "KDChartMeasure.h"

#include <KDABLibFakes>

using namespace KDChart;

#define d d_func()

AbstractAxis::Private::Private( AbstractDiagram* diagram, AbstractAxis* axis )
    : observer( 0 )
    , mDiagram( diagram )
    , mAxis( axis )
{
    // Note: We do NOT call setDiagram( diagram, axis );
    //       but it is called in AbstractAxis::delayedInit() instead!
}

AbstractAxis::Private::~Private()
{
    delete observer;
    observer = 0;
}

bool AbstractAxis::Private::setDiagram( AbstractDiagram* diagram_, bool delayedInit )
{
    AbstractDiagram* diagram = delayedInit ? mDiagram : diagram_;
    if ( delayedInit ) {
        mDiagram = 0;
    }

    // do not set a diagram again that was already set
    if ( diagram &&
        ((diagram == mDiagram) || secondaryDiagrams.contains( diagram )) )
        return false;

    bool bNewDiagramStored = false;
    if ( ! mDiagram ) {
        mDiagram = diagram;
        delete observer;
        if ( mDiagram ) {
            observer = new DiagramObserver( mDiagram, mAxis );
            const bool con = connect( observer, SIGNAL( diagramDataChanged( AbstractDiagram *) ),
                    mAxis, SIGNAL( coordinateSystemChanged() ) );
            Q_UNUSED( con )
            Q_ASSERT( con );
            bNewDiagramStored = true;
        } else {
            observer = 0;
        }
    } else {
        if ( diagram )
            secondaryDiagrams.enqueue( diagram );
    }
    return bNewDiagramStored;
}

void AbstractAxis::Private::unsetDiagram( AbstractDiagram* diagram )
{
    if ( diagram == mDiagram ) {
        mDiagram = 0;
        delete observer;
        observer = 0;
    } else {
        secondaryDiagrams.removeAll( diagram );
    }
    if ( !secondaryDiagrams.isEmpty() ) {
        AbstractDiagram *nextDiagram = secondaryDiagrams.dequeue();
        setDiagram( nextDiagram );
    }
}

bool AbstractAxis::Private::hasDiagram( AbstractDiagram* diagram ) const
{
    return diagram == mDiagram || secondaryDiagrams.contains( diagram );
}

void AbstractAxis::Private::updateLayouts()
{
    if ( CartesianAxis* cartesianAxis = qobject_cast< CartesianAxis* >( mAxis ) ) {
        cartesianAxis->layoutPlanes();
    } else {
        mAxis->update();
    }
}

AbstractAxis::AbstractAxis ( AbstractDiagram* diagram )
    : AbstractArea( new Private( diagram, this ) )
{
    init();
    QTimer::singleShot(0, this, SLOT(delayedInit()));
}

AbstractAxis::~AbstractAxis()
{
    d->mDiagram = 0;
    d->secondaryDiagrams.clear();
}


void AbstractAxis::init()
{
    Measure m( 14, KDChartEnums::MeasureCalculationModeAuto, KDChartEnums::MeasureOrientationAuto );
    d->textAttributes.setFontSize( m );
    m.setValue( 6 );
    m.setCalculationMode( KDChartEnums::MeasureCalculationModeAbsolute );
    d->textAttributes.setMinimalFontSize( m );
    if ( d->diagram() )
        createObserver( d->diagram() );
}

void AbstractAxis::delayedInit()
{
    // We call setDiagram() here, because the c'tor of Private
    // only has stored the pointers, but it did not call setDiagram().
    if ( d )
        d->setDiagram( 0, true /* delayedInit */ );
}

bool AbstractAxis::compare( const AbstractAxis* other ) const
{
    if ( other == this ) {
        return true;
    }
    if ( !other ) {
        return false;
    }

    return  ( static_cast<const AbstractAreaBase*>(this)->compare( other ) ) &&
            (textAttributes() == other->textAttributes()) &&
            (labels()         == other->labels()) &&
            (shortLabels()    == other->shortLabels());
}


const QString AbstractAxis::customizedLabel( const QString& label ) const
{
    return label;
}


void AbstractAxis::createObserver( AbstractDiagram* diagram )
{
    d->setDiagram( diagram );
}

void AbstractAxis::deleteObserver( AbstractDiagram* diagram )
{
    d->unsetDiagram( diagram );
}

void AbstractAxis::connectSignals()
{
    if ( d->observer ) {
        const bool con = connect( d->observer, SIGNAL( diagramDataChanged( AbstractDiagram *) ),
                this, SIGNAL( coordinateSystemChanged() ) );
        Q_UNUSED( con );
        Q_ASSERT( con );
    }
}

void AbstractAxis::setTextAttributes( const TextAttributes &a )
{
    if ( d->textAttributes == a )
        return;

    d->textAttributes = a;
    d->updateLayouts();
}

TextAttributes AbstractAxis::textAttributes() const
{
    return d->textAttributes;
}


void AbstractAxis::setRulerAttributes( const RulerAttributes &a )
{
    d->rulerAttributes = a;
    d->updateLayouts();
}

RulerAttributes AbstractAxis::rulerAttributes() const
{
    return d->rulerAttributes;
}

void AbstractAxis::setLabels( const QStringList& list )
{
    if ( d->hardLabels == list )
        return;

    d->hardLabels = list;
    d->updateLayouts();
}

QStringList AbstractAxis::labels() const
{
    return d->hardLabels;
}

void AbstractAxis::setShortLabels( const QStringList& list )
{
    if ( d->hardShortLabels == list )
        return;

    d->hardShortLabels = list;
    d->updateLayouts();
}

QStringList AbstractAxis::shortLabels() const
{
    return d->hardShortLabels;
}

const AbstractCoordinatePlane* AbstractAxis::coordinatePlane() const
{
    if ( d->diagram() )
        return d->diagram()->coordinatePlane();
    return 0;
}

const AbstractDiagram * KDChart::AbstractAxis::diagram() const
{
    return d->diagram();
}

bool KDChart::AbstractAxis::observedBy( AbstractDiagram * diagram ) const
{
    return d->hasDiagram( diagram );
}

void KDChart::AbstractAxis::update()
{
    if ( d->diagram() )
        d->diagram()->update();
}
