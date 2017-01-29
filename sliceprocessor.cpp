#include "sliceprocessor.h"

#include <QDebug>
#include <QImage>
#include <QColor>
#include "math.h"

#define DEG2RAD  0.01745329251



SliceProcessor::SliceProcessor(QObject *parent) :
    QThread(parent)
{
    preview = true;
    blending = 1.0f;
    reverse = false;
    angle = 40;
    scale_x = 1;
    scale_y = 1;
}

// QSize thumbnail = size.scaled(200, 200, Qt::KeepAspectRatio);

float SliceProcessor::blend(float pos) {

    if (blending < 0.01) {
        return pos < 1 ? 1 : 0;
    }

    float s = 0.5 - (abs(pos) - 1) * (1. / (2. * blending));

    // Clamp value between 0 and 1
    if (s < 0.0)
        s = 0.0;
    else if (s > 1.0)
        s = 1.0;

    // Reserve luminance
    s /= sqrt(s * s + (1 - s) * (1 - s));

    return s;
}




void SliceProcessor::run() {

    if (!preview)
        emit info("Started");

    if (images.size() < 2) {
        emit error("Too few images!");
        return;
    }
    blending = 1.0f;

    QImage output;

    if (preview) {

        // TODO: Better checking here!
        if (thumbs.empty()) {
            qDebug() << "Generating smaller images for preview...";

            if (images.size() > 20) {
                // TODO: drop the number of images if they are
            }

            for (int i = 0; i < images.size(); i++) {
                QImage im(images[i]);
                thumbs.append(im.scaled(800, 800, Qt::KeepAspectRatio));
            }
        }
        output = QImage(thumbs[0].size(), QImage::Format_RGB32);

    }
    else {

        qDebug() << "Starting final rendering..";
        qDebug() << images.size() << "images to be procesed!";

        output = QImage(images[0]);
    }





    // Rotation
    float sn = sin(DEG2RAD * angle);
    float cn = cos(DEG2RAD * angle);

    //float w = output.width() / images.size();
    float w = 1 / images.size();

    QImage layer;

    int i, j;
    for (int p = 0; p < images.size(); ++p) {

        // Layer indexing
        int pi = reverse ? (images.size() - p - 1) : p;

        if (preview) {
            layer = thumbs[pi];
        }
        else {
            // Load full resolution image
            qDebug() << "Processing" << images[pi];
            layer = QImage(images[pi]);

            if (layer.size() != output.size()) {
                emit error(QString("Image %s is wrong size!").arg(images[p]));
                return;
            }
        }


        QColor r, o;

        for (i = 0; i < output.width(); ++i) {
        for (j = 0; j < output.height(); ++j) {

            // To [-1; 1] space
            float ox = (float)i / output.width() - 0.5;
            float oy = (float)j / output.height() - 0.5;

            // Transform (rotation + scaling)
            float cx = scale_x * cn * ox - scale_x * sn * oy;
            float cy = scale_y * sn * ox + scale_y * cn * oy;

            // TODO: moar stuff here!
            // non linear transform?

            // Convert back to pixel space
            int px = (cx + 0.5) * output.width();
            int py = (cy + 0.5) * output.height();

            // Fiqure out should we copy the pixel to the final version or not
            /*if (px < int(w*p) + 1 * oy && p > 0)
                continue;
            if (px < int(w*(p+1)) + 1 + 1 * oy && p != images.size())
                continue;*/
            float b = 1;


            if (pi == 0 && cx < -0.5) {

            }
            else if (pi == images.size() - 1&& cx > 0.5) {

            }
            else {

                b = blend(2*((images.size()-1) * (cx + 0.5) - pi - 1));
                if (b < 0.01)
                    continue;
            }

            // Sample the layer
            r = layer.pixel(i, j);

            if (1) {
                output.setPixel(i, j, r.rgb());
            }
            else {
                //b = 0.3;
                o = output.pixel(i, j);
                output.setPixel(i, j, qRgb(o.red() + b * r.red(), o.green() + b * r.green(), o.blue() + b * r.blue()));
            }

        }
        }


        if (preview) {
            preview_image = output.copy();
            emit preview_ready();
        }
        else {
            emit progress(p);
        }

        if (isInterruptionRequested()) {
            return;
        }

    }


    if (preview) {

    }
    else {
        qDebug() << "Exporting \"time.jpg\"";
        output.save("time.jpg", "jpg", 90);
        emit info("Done!");
    }

}
