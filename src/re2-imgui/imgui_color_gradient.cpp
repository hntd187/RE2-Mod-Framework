//
//  imgui_color_gradient.cpp
//  imgui extension
//
//  Created by David Gallardo on 11/06/16.


#include "imgui_color_gradient.h"
#include "imgui_internal.h"

ImGradient::~ImGradient()
{
	for (ImGradientMark* mark : m_marks)
	{
		delete mark;
	}
}

void ImGradient::addMark(float position, ImColor const color)
{
    position = ImClamp(position, 0.0f, 1.0f);
	ImGradientMark* newMark = new ImGradientMark();
    newMark->position = position;
    newMark->color[0] = color.Value.x;
    newMark->color[1] = color.Value.y;
    newMark->color[2] = color.Value.z;
    
    m_marks.push_back(newMark);
    
    refreshCache();
}

void ImGradient::removeMark(ImGradientMark* mark)
{
    m_marks.remove(mark);
    refreshCache();
}

void ImGradient::getColorAt(float position, float* color) const
{
    position = ImClamp(position, 0.0f, 1.0f);    
    int cachePos = (position * 255);
    cachePos *= 3;
    color[0] = m_cachedValues[cachePos+0];
    color[1] = m_cachedValues[cachePos+1];
    color[2] = m_cachedValues[cachePos+2];
}

void ImGradient::computeColorAt(float position, float* color) const
{
    position = ImClamp(position, 0.0f, 1.0f);
    
    ImGradientMark* lower = nullptr;
    ImGradientMark* upper = nullptr;
    
    for(ImGradientMark* mark : m_marks)
    {
        if(mark->position < position)
        {
            if(!lower || lower->position < mark->position)
            {
                lower = mark;
            }
        }
        
        if(mark->position >= position)
        {
            if(!upper || upper->position > mark->position)
            {
                upper = mark;
            }
        }
    }
    
    if(upper && !lower)
    {
        lower = upper;
    }
    else if(!upper && lower)
    {
        upper = lower;
    }
    else if(!lower && !upper)
    {
        color[0] = color[1] = color[2] = 0;
        return;
    }
    
    if(upper == lower)
    {
        color[0] = upper->color[0];
        color[1] = upper->color[1];
        color[2] = upper->color[2];
    }
    else
    {
        float distance = upper->position - lower->position;
        float delta = (position - lower->position) / distance;
        
        //lerp
        color[0] = ((1.0f - delta) * lower->color[0]) + ((delta) * upper->color[0]);
        color[1] = ((1.0f - delta) * lower->color[1]) + ((delta) * upper->color[1]);
        color[2] = ((1.0f - delta) * lower->color[2]) + ((delta) * upper->color[2]);
    }
}

void ImGradient::refreshCache()
{
    m_marks.sort([](const ImGradientMark * a, const ImGradientMark * b) { return a->position < b->position; });
    
    for(int i = 0; i < 256; ++i)
    {
        computeColorAt(i/255.0f, &m_cachedValues[i*3]);
    }
}